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

    struct KeyframeGroup
    {
        std::vector<AnimationTransformation> joints;
        int timeStampsAccessor;
    };

    std::vector<KeyframeGroup> groups;

    struct JointTransformations
    {
        uint32_t joint;
        ETransformation transformation;
        BufferSegment bufferSegment;
    };

    std::vector<JointTransformations> channelBuffers;

    // Group channels by timestamps they need to follow
    // Each channel is responsible for a single joint/node
    for (auto& channel : gltfAnim.channels)
    {
        auto sampler = gltfAnim.samplers[channel.sampler];
        std::vector<KeyframeGroup>::iterator iter = std::find_if(groups.begin(), groups.end(), [&sampler](KeyframeGroup& a_Entry) {return a_Entry.timeStampsAccessor == sampler.input; });
        if (iter == groups.end())
        {
            groups.emplace_back();
            groups.back().timeStampsAccessor = sampler.input;
            iter = groups.end() - 1;
        }
        (*iter).joints.emplace_back();
        (*iter).joints.back().m_TargetJoint = channel.target.node;
        if (channel.target.path == "translation")
            (*iter).joints.back().m_Transformation = Translate;
        else if (channel.target.path == "rotation")
            (*iter).joints.back().m_Transformation = Rotate;
        else if (channel.target.path == "scale")
            (*iter).joints.back().m_Transformation = Scale;

        channelBuffers.emplace_back(); 
        channelBuffers.back().transformation = (*iter).joints.back().m_Transformation;
        channelBuffers.back().joint = channel.target.node;
        channelBuffers.back().bufferSegment = GetSegmentFromAccessor(a_Doc, sampler.output);
    }
       
    Animation anim;
    anim.m_Name = gltfAnim.name;
    for (KeyframeGroup& group : groups)
    {
        auto bufferSegment = GetSegmentFromAccessor(a_Doc, group.timeStampsAccessor);
        auto timeStamps = ParseBytes<float>(bufferSegment);

        for (float timeStamp : timeStamps)
        {
            anim.m_Keyframes.emplace_back();
            anim.m_Keyframes.back().m_Time = timeStamp;
            anim.m_Keyframes.back().m_AffectedJoints = group.joints;
        }
    }

    for (auto& channelBuffer : channelBuffers)
    {
        auto parsed = ParseBytes<float>(channelBuffer.bufferSegment, channelBuffer.bufferSegment.numComponents);

        for (size_t i = 0; i < channelBuffer.bufferSegment.count; i++)
        {
            Keyframe& keyframe = anim.m_Keyframes[i];
            auto iter = std::find_if(keyframe.m_AffectedJoints.begin(), keyframe.m_AffectedJoints.end(), [&channelBuffer](AnimationTransformation& a_Entry)
                {return a_Entry.m_TargetJoint == channelBuffer.joint && a_Entry.m_Transformation == channelBuffer.transformation; });

            auto& transformation = *iter;

            switch (transformation.m_Transformation)
            {
            case Translate:
            case Scale:
                transformation.m_DeltaPosition = Vector3(parsed[i * 3 + 0], parsed[i * 3 + 1], parsed[i * 3 + 2]);
                break;
            case Rotate:
                transformation.m_DeltaRotation = Quaternion(parsed[i * 3 + 0], parsed[i * 3 + 1], parsed[i * 3 + 2], parsed[i * 3 + 3]);

                break;
            }
        }

    }

    return anim;
}

Mesh& ModelLoader::LoadMeshFromGLTF(std::string a_FilePath)
{
    fg::Document doc = fg::LoadFromText(a_FilePath);

    auto mesh = doc.meshes[0];




    m_LoadedMeshes.emplace(a_FilePath, Mesh());

    Mesh& m = m_LoadedMeshes[a_FilePath];
    m.m_Skeleton = CreateSkeleton(doc, 0);
    auto anim = GetAnimation(doc, 0);
    m.m_Skeleton.UpdateSkeleton(anim, 0.0f);

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