#include "ModelLoader.h"
#include "ServiceLocator.h"
#include "Device.h"
#include "CommandQueue.h"
#include "GraphicsCommandList.h"
#include "SimpleMath.h"

namespace fg = fx::gltf;
using namespace DirectX::SimpleMath;

uint8_t GetNumberComponents(fg::Accessor& a_Accessor)
{
    switch (a_Accessor.type)
    {
    case fg::Accessor::Type::Scalar:
        return 1;
    case fg::Accessor::Type::Vec2:
        return 2;
    case fg::Accessor::Type::Vec3:
        return 3;
    case fg::Accessor::Type::Vec4:
    case fg::Accessor::Type::Mat2:
        return 4;
    case fg::Accessor::Type::Mat3:
        return 9;
    case fg::Accessor::Type::Mat4:
        return 16;
    default:
        std::cout << "ERROR: Unknown attribute type, cannot load model" << std::endl;
        return 0;
    }
}

size_t GetComponentSize(fg::Accessor a_Accessor)
{
    switch (a_Accessor.componentType)
    {
    case fx::gltf::Accessor::ComponentType::Byte:
    case fx::gltf::Accessor::ComponentType::UnsignedByte:
        return 1;
    case fx::gltf::Accessor::ComponentType::Short:
    case fx::gltf::Accessor::ComponentType::UnsignedShort:
        return 2;
    case fx::gltf::Accessor::ComponentType::UnsignedInt:
    case fx::gltf::Accessor::ComponentType::Float:
        return 4;
    default:
        std::cout << "ERROR: Unknown attribute type, cannot load model" << std::endl;
        return 0;
    }
}

struct BufferSegment
{
    BufferSegment(){};
    BufferSegment(fg::Buffer a_Buffer, fg::BufferView a_View, fg::Accessor a_Accessor)
        : buffer(a_Buffer)
        , offset(a_View.byteOffset + a_Accessor.byteOffset)
        , stride(a_View.byteStride)
        , componentSize(GetComponentSize(a_Accessor))
        , numComponents(GetNumberComponents(a_Accessor))
        , count(a_Accessor.count)
    {}
    fg::Buffer buffer;
    uint32_t offset;
    uint32_t stride;
    size_t componentSize;
    uint8_t numComponents;
    uint32_t count;
};


template<typename T>
std::vector<T> ParseBytes(BufferSegment& a_Buffer, uint8_t a_NumComponentsForce = 1)
{
    a_NumComponentsForce;
    uint32_t inBufferOffset = a_Buffer.offset;

    std::vector<T> parsedBuffer;

    for (uint32_t i = 0; i < a_Buffer.count; ++i)
    {
        for (uint8_t component = 0; component < a_Buffer.componentSize * a_Buffer.numComponents / sizeof(T); component++)
        {
            union
            {
                uint8_t bytes[sizeof(T)];
                T parsed;
            };

            for (size_t byte = 0; byte < sizeof(T); byte++)
            {
                bytes[byte] = a_Buffer.buffer.data[inBufferOffset + component * sizeof(T) + byte];
            }
            parsedBuffer.push_back(parsed);
        }
        inBufferOffset += a_Buffer.stride;
    }

    return parsedBuffer;
}

BufferSegment GetSegmentFromAccessor(fg::Document& a_Doc, size_t a_AccessorID)
{
    auto accessor = a_Doc.accessors[a_AccessorID];
    auto bufferView = a_Doc.bufferViews[accessor.bufferView];
    auto buffer = a_Doc.buffers[bufferView.buffer];

    auto out = BufferSegment(buffer, bufferView, accessor);
    return out;
}



Joint ConstructJoints(fg::Document a_Doc, uint32_t a_ParentJoint)
{
    auto node = a_Doc.nodes[a_ParentJoint];

    Joint joint;

    for (auto child : node.children)
    {
        joint.m_Children.push_back(ConstructJoints(a_Doc, child));
    }

    joint.id = a_ParentJoint;
    joint.name = node.name;
    joint.pos = Vector3(node.translation[0], node.translation[1], node.translation[2]);
    joint.rotation = Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
    joint.scale = Vector3(node.scale[0], node.scale[1], node.scale[2]);

    return joint;
}


Skeleton CreateSkeleton(fg::Document a_Doc, uint32_t a_SkinID)
{
    auto skin = a_Doc.skins[a_SkinID];

    auto segment = GetSegmentFromAccessor(a_Doc, skin.inverseBindMatrices);
    auto IBM = ParseBytes<DirectX::XMMATRIX>(segment);
    int IBMIndexCounter = 0;

    Skeleton skeleton;
    skeleton.m_RootJoint = std::make_unique<Joint>(ConstructJoints(a_Doc, skin.skeleton));

    skeleton.m_RootJoint->CollectJoints(skeleton.m_Joints);

    for (auto j : skeleton.m_Joints)
    {       
        // because why fucking not
        j->inverseBindMatrix = Matrix(IBM[IBMIndexCounter]);
        IBMIndexCounter ++;
    }

    // is this needed? need to test with more samples, mainly from artists to know how maya does the exporting
    std::sort(skeleton.m_Joints.begin(), skeleton.m_Joints.end(), [](Joint*& a_Left, Joint*& a_Right) {return a_Left->id < a_Right->id; });


    return skeleton;
}

Animation GetAnimation(fg::Document a_Doc, int a_AnimID)
{
    auto gltfAnim = a_Doc.animations[a_AnimID];
    
    Animation finalAnim;
    finalAnim.m_Name = gltfAnim.name;

    std::map<size_t, size_t> alreadyLoadedTimestamps;
    finalAnim.m_TimestampLists;

    float length = 0.0f;

    for (auto& ch : gltfAnim.channels)
    {
        auto sampler = gltfAnim.samplers[ch.sampler];

        auto timestampAccessor = sampler.input;
        {
            if (std::find_if(alreadyLoadedTimestamps.begin(), alreadyLoadedTimestamps.end(),
                [timestampAccessor](std::pair<size_t, size_t> a_Entry) {return a_Entry.first == timestampAccessor; }) == alreadyLoadedTimestamps.end())
            {
                alreadyLoadedTimestamps.emplace(timestampAccessor, finalAnim.m_TimestampLists.size());
                auto segment = GetSegmentFromAccessor(a_Doc, timestampAccessor);
                finalAnim.m_TimestampLists.push_back(ParseBytes<float>(segment));
                length = std::fmax(length, finalAnim.m_TimestampLists.back().back());
            }
        }

        finalAnim.m_Channels.emplace_back();
        auto& animChannel = finalAnim.m_Channels.back();

        animChannel.m_JointID = ch.target.node;
        animChannel.m_Timestamps = &finalAnim.m_TimestampLists[alreadyLoadedTimestamps[timestampAccessor]];

        if (ch.target.path == "translation")
            animChannel.m_TransformationType = Translate;
        else if (ch.target.path == "rotation")
            animChannel.m_TransformationType = Rotate;
        else if (ch.target.path == "scale")
            animChannel.m_TransformationType = Scale;

        if (sampler.interpolation == fx::gltf::Animation::Sampler::Type::Linear)
            animChannel.m_InterpolationMethod = Linear;
        else if (sampler.interpolation == fx::gltf::Animation::Sampler::Type::Step)
            animChannel.m_InterpolationMethod = Step;
        else if (sampler.interpolation == fx::gltf::Animation::Sampler::Type::CubicSpline)
            animChannel.m_InterpolationMethod = CubicSpline;

        auto transformationAccessor = sampler.output;
        auto transformationsSegment = GetSegmentFromAccessor(a_Doc, transformationAccessor);
        auto transformationFloats = ParseBytes<float>(transformationsSegment);

        for (size_t i = 0; i < animChannel.m_Timestamps->size(); i++)
        {
            animChannel.m_TransformationKeyframes.emplace_back();
            auto& transformation = animChannel.m_TransformationKeyframes.back();
            switch (animChannel.m_TransformationType)
            {
            case Translate:
            case Scale:
                transformation.m_DeltaPosition = Vector3(&transformationFloats[i * 3]);
                break;
            case Rotate:
                transformation.m_DeltaRotation = Quaternion(&transformationFloats[i * 4]);
                break;
            }
        }

    }


    finalAnim.m_Length = length;
    return finalAnim;
}
Mesh& ModelLoader::LoadMeshFromGLTF(std::string a_FilePath)
{
    fg::Document doc = fg::LoadFromText(a_FilePath);

    auto mesh = doc.meshes[0];





    m_LoadedMeshes.emplace(a_FilePath, Mesh());

    Mesh& m = m_LoadedMeshes[a_FilePath];
    m.m_Skeleton = CreateSkeleton(doc, 0);
    m.m_Animation = GetAnimation(doc, 2);

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT2> texCoords;
    std::vector<DirectX::XMFLOAT4> weights;
    std::vector<DirectX::XMUINT4> jointIDs;
    
    for (auto primitive : mesh.primitives)
    {
        for (auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);                
                positions = ParseBytes<DirectX::XMFLOAT3>(segment);
            }
            else if (attrib.first == "TEXCOORD_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                texCoords = ParseBytes<DirectX::XMFLOAT2>(segment);
            }
            else if (attrib.first == "JOINTS_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                auto jointIDsUshort = ParseBytes<unsigned short int>(segment);

                for (size_t i = 0; i < jointIDsUshort.size(); i += 4)
                {
                    jointIDs.emplace_back(jointIDsUshort[i + 0], jointIDsUshort[i + 1], jointIDsUshort[i + 2], jointIDsUshort[i + 3]);
                }
            }
            else if (attrib.first == "WEIGHTS_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                weights = ParseBytes<DirectX::XMFLOAT4>(segment);
            }
        }
    }

    struct vertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 tex;
    };



    auto copyCommandQueue = m_Services->m_Device->GetCommandQueue();
    auto copyCommandList = copyCommandQueue->GetCommandList();

    m.m_Positions = copyCommandList->CreateVertexBuffer(positions);
    m.m_TexCoords = copyCommandList->CreateVertexBuffer(texCoords);
    m.m_JointIDs = copyCommandList->CreateVertexBuffer(jointIDs);
    m.m_Weights = copyCommandList->CreateVertexBuffer(weights);


    copyCommandQueue->ExecuteCommandList(*copyCommandList);

    return m;
}