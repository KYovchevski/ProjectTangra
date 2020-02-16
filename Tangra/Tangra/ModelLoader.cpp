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
struct BufferSegment
{
    BufferSegment(){};
    BufferSegment(fg::Buffer a_Buffer, fg::BufferView a_View, fg::Accessor a_Accessor)
        : buffer(a_Buffer)
        , offset(a_View.byteOffset + a_Accessor.byteOffset)
        , stride(a_View.byteStride)
        , componentNumber(GetNumberComponents(a_Accessor))
        , count(a_Accessor.count)
    {}
    fg::Buffer buffer;
    uint32_t offset;
    uint32_t stride;
    uint8_t componentNumber;
    uint32_t count;
};


struct Joint
{
    UINT id;
    std::string name;
    Vector3 pos;
    Quaternion rotation;
    Vector3 scale;
};

struct Skeleton
{
    std::vector<Joint> m_Joints;
    std::vector<uint32_t> m_JointIDs;

    uint32_t m_RootJoint;
};

enum ETransformation
{
    Translate = 0,
    Rotate = 1,
    Scale = 2
};

struct AnimationTransformation
{
    AnimationTransformation(){
        m_DeltaRotation = Quaternion::Identity;
    }
    uint32_t m_TargetJoint;
    ETransformation m_Transformation;

    union
    {
        Vector3 m_DeltaPosition;
        Quaternion m_DeltaRotation;
        Vector3 m_DeltaScale;
    };
};

struct Keyframe
{
    float m_Time;
    std::vector<AnimationTransformation> m_AffectedJoints;
};

struct Animation
{
    std::string m_Name;
    std::vector<Keyframe> m_Keyframes;
};

Skeleton CreateSkeleton(fg::Document a_Doc, uint32_t a_SkinID)
{
    auto skin = a_Doc.skins[a_SkinID];

    Skeleton skeleton;
    skeleton.m_RootJoint = skin.skeleton;

    for (auto j : skin.joints)
    {
        auto node = a_Doc.nodes[j];

        skeleton.m_Joints.emplace_back();
        Joint& joint = skeleton.m_Joints.back();

        joint.id = j;
        joint.name = node.name;
        joint.pos = Vector3(node.translation[0], node.translation[1], node.translation[2]);
        joint.rotation = Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
        joint.scale = Vector3(node.scale[0], node.scale[1], node.scale[2]);
    }

    std::sort(skeleton.m_Joints.begin(), skeleton.m_Joints.end(), [](Joint& a_Left, Joint& a_Right) {return a_Left.id < a_Right.id; });

    for (Joint& j : skeleton.m_Joints)
    {
        skeleton.m_JointIDs.push_back(j.id);
    }

    return skeleton;
}

template<typename T>
std::vector<T> ParseBytes(BufferSegment& a_Buffer)
{
    uint32_t inBufferOffset = a_Buffer.offset;

    std::vector<T> parsedBuffer;

    for (uint32_t i = 0; i < a_Buffer.count; ++i)
    {
        for (uint8_t component = 0; component < a_Buffer.componentNumber; component++)
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
    a_Buffer.buffer.data[a_Buffer.offset];

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
        auto parsed = ParseBytes<float>(channelBuffer.bufferSegment);

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

    doc.skins[0].skeleton; // ID of root bone's node

    doc.animations[0].channels[0].target.node; // ID of bone (node)
    doc.animations[0].channels[0].target.path; // transform type
    doc.animations[0].channels[0].sampler; // ID of sampler

    doc.accessors[doc.animations[0].samplers[0].input]; // timestamps accessor
    doc.accessors[doc.animations[0].samplers[0].output]; // actual transformation access

    auto timestampsSegment = GetSegmentFromAccessor(doc, doc.animations[0].samplers[0].input);

    auto timestamps = ParseBytes<float>(timestampsSegment);
    auto transformationsSegment = GetSegmentFromAccessor(doc, doc.animations[0].samplers[0].output);

    auto transformations = ParseBytes<float>(transformationsSegment);

    auto skele = CreateSkeleton(doc, 0);

    auto skin = doc.skins[0];

    auto anim = GetAnimation(doc, 0);

    std::vector<Joint> joints;

    for (auto j : skin.joints)
    {
        auto node = doc.nodes[j];        

        joints.emplace_back();
        joints.back().id = j;
        joints.back().name = node.name;
        joints.back().pos = Vector3(node.translation[0], node.translation[1], node.translation[2]);
        joints.back().rotation = Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
        joints.back().scale = Vector3(node.scale[0], node.scale[1], node.scale[2]);
    }

    std::vector<float> positions, texCoords, weights;
    std::vector<unsigned short int> jointIDs;
    
    for (auto primitive : mesh.primitives)
    {        
        for (auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);                
                positions = ParseBytes<float>(segment);
            }
            else if (attrib.first == "TEXCOORD_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                texCoords = ParseBytes<float>(segment);
            }
            else if (attrib.first == "JOINTS_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                jointIDs = ParseBytes<unsigned short int>(segment);
            }
            else if (attrib.first == "WEIGHTS_0")
            {
                BufferSegment segment = GetSegmentFromAccessor(doc, attrib.second);
                weights = ParseBytes<float>(segment);
            }
        }
    }

    struct vertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 tex;
    };

    std::vector<vertex> interleaved;

    for (size_t i = 0; i < positions.size() / 3; i++)
    {
        interleaved.emplace_back();
        interleaved.back().pos.x = positions[i * 3 + 0];
        interleaved.back().pos.y = positions[i * 3 + 1];
        interleaved.back().pos.z = positions[i * 3 + 2];

        interleaved.back().tex.x = texCoords[i * 2 + 0];
        interleaved.back().tex.y = texCoords[i * 2 + 1];

    }

    auto copyCommandQueue = m_Services->m_Device->GetCommandQueue();
    auto copyCommandList = copyCommandQueue->GetCommandList();

    Mesh m;
    
    m.m_VertexBuffer = copyCommandList->CreateVertexBuffer(interleaved);

    copyCommandQueue->ExecuteCommandList(*copyCommandList);

    m_LoadedMeshes.emplace(a_FilePath, m);
    return m_LoadedMeshes[a_FilePath];
}