#pragma once
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;
struct Animation;

struct Joint
{
    void CollectJoints(std::vector<Joint*>& a_Joints)
    {
        a_Joints.push_back(this);
        for (auto& child : m_Children)
        {
            child.CollectJoints(a_Joints);
        }
    }

    void UpdateJoints(Animation& a_Animation, float a_Timestamp, const Matrix& a_ParentTransform = Matrix::Identity);
    UINT id;
    std::string name;

    std::vector<Joint> m_Children;

    Matrix Transform;
    Matrix inverseBindMatrix;
    Vector3 pos;
    Quaternion rotation;
    Vector3 scale;
};

enum ETransformation
{
    Translate = 0,
    Rotate = 1,
    Scale = 2
};

enum EInterpolationMethod
{
    Linear = 0,
    Step = 1,
    CubicSpline = 2
};

struct AnimationTransformation
{
    AnimationTransformation() {
        m_DeltaRotation = Quaternion::Identity;
    }
    uint32_t m_TargetJoint;

    union
    {
        Vector3 m_DeltaPosition;
        Quaternion m_DeltaRotation;
        Vector3 m_DeltaScale;
    };
};

struct Channel
{

    AnimationTransformation GetInterpolatedTransformAtTimestamp(float a_Timestamp);

    uint32_t m_JointID;
    std::vector<float>* m_Timestamps;
    std::vector<AnimationTransformation> m_TransformationKeyframes;
    ETransformation m_TransformationType;
    EInterpolationMethod m_InterpolationMethod;
};

struct Animation
{
    std::string m_Name;
    float m_Length;

    std::vector<std::vector<float>> m_TimestampLists;

    std::vector<Channel> m_Channels;
};


struct Skeleton
{
    void UpdateSkeleton(Animation& a_Anim, float a_Timestamp)
    {
        a_Anim, a_Timestamp;
        Matrix transform = Matrix::Identity;
        
        m_RootJoint->UpdateJoints(a_Anim, a_Timestamp, transform);
    }

    std::vector<DirectX::SimpleMath::Matrix> GetMatrices()
    {
        std::vector<Matrix> output;
        for (Joint* joint : m_Joints)
        {
            output.push_back(joint->Transform);
        }

        return std::move(output);
    }

    std::vector<Joint*> m_Joints;
    std::vector<uint32_t> m_JointIDs;

    std::unique_ptr<Joint> m_RootJoint;
};

struct Mesh
{
    VertexBuffer m_Positions;
    VertexBuffer m_TexCoords;
    VertexBuffer m_JointIDs;
    VertexBuffer m_Weights;

    IndexBuffer m_IndexBuffer;

    Texture m_Texture;

    Skeleton m_Skeleton;
    Animation m_Animation;
};

inline void Joint::UpdateJoints(Animation& a_Animation, float a_Timestamp, const Matrix& a_ParentTransform)
{

    // Local Transform
    Matrix transform = Matrix::Identity;

    std::vector<Channel*> affectingChannels;
    for (auto& channel : a_Animation.m_Channels)
    {
        if (channel.m_JointID == id)
        {
            affectingChannels.push_back(&channel);
        }
    }

    for (Channel* affectingChannel : affectingChannels)
    {
        auto interpolated = affectingChannel->GetInterpolatedTransformAtTimestamp(a_Timestamp);

        switch (affectingChannel->m_TransformationType)
        {
        case Translate:
            pos = interpolated.m_DeltaPosition;
            break;
        case Rotate:
            rotation = interpolated.m_DeltaRotation;
            break;
        case Scale:
            scale = interpolated.m_DeltaScale;
            break;
        }
    }

    transform *= Matrix::CreateScale(scale);
    transform *= Matrix::CreateFromQuaternion(rotation);
    transform *= Matrix::CreateTranslation(pos);

    // Parent Transform
    transform = transform * a_ParentTransform;

    // Vertex transform
    Transform = inverseBindMatrix * transform;
    for (auto& child : m_Children)
    {
        child.UpdateJoints(a_Animation, a_Timestamp, transform);
    }

}

inline AnimationTransformation Channel::GetInterpolatedTransformAtTimestamp(float a_Timestamp)
{
    for (size_t i = 0; i < m_Timestamps->size(); i++)
    {
        float before = (*m_Timestamps)[i];
        float after = (*m_Timestamps)[(i + 1) % m_Timestamps->size()];

        if (before <= a_Timestamp && after > a_Timestamp)
        {
            float alpha = (a_Timestamp - before) / (after - before);

            AnimationTransformation output;

            switch (m_TransformationType)
            {
            case Translate:
                output.m_DeltaPosition = Vector3::Lerp(m_TransformationKeyframes[i].m_DeltaPosition,
                    m_TransformationKeyframes[(i + 1) % m_Timestamps->size()].m_DeltaPosition, alpha);
                return output;
            case Rotate:
                output.m_DeltaRotation = Quaternion::Slerp(m_TransformationKeyframes[i].m_DeltaRotation,
                    m_TransformationKeyframes[(i + 1) % m_Timestamps->size()].m_DeltaRotation, alpha);
                return output;
            case Scale:
                output.m_DeltaScale = Vector3::Lerp(m_TransformationKeyframes[i].m_DeltaScale,
                    m_TransformationKeyframes[(i + 1) % m_Timestamps->size()].m_DeltaScale, alpha);
                return output;
            }

        }
    }
    return AnimationTransformation();
}
