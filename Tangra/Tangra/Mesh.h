#pragma once
#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "Texture.h"
#include "SimpleMath.h"

using namespace DirectX::SimpleMath;

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

    void UpdateJoints(const Matrix& a_ParentTransform)
    {
        // Local Transform
        Matrix transform = Matrix::Identity;

        //if (name == "b_RightLeg01_019")
        //{
        //    //std::cout << "k" << std::endl;
        //}

        transform *= Matrix::CreateScale(scale);
        transform *= Matrix::CreateFromQuaternion(rotation);
        transform *= Matrix::CreateTranslation(pos);

        // Parent Transform
        transform = transform * a_ParentTransform;

        // Vertex transform
        Transform = inverseBindMatrix * transform;
        for (auto& child : m_Children)
        {
            child.UpdateJoints(transform);
        }
    }
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

struct AnimationTransformation
{
    AnimationTransformation() {
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


struct Skeleton
{
    void UpdateSkeleton(Animation& a_Anim, float a_Timestamp)
    {
        a_Anim, a_Timestamp;
        Matrix transform = Matrix::Identity;


        //std::cout << m_RootJoint->m_Children[0].m_Children[0].m_Children[2].name << std::endl;
        m_RootJoint->m_Children[0].m_Children[0].m_Children[2].pos = Vector3(0.0f, 0.0f, -10.0f);
        //m_RootJoint->rotation = Quaternion::CreateFromYawPitchRoll(0.0f, DirectX::XMConvertToRadians(10.0f), 0.0f);
        m_RootJoint->UpdateJoints(transform);
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
};
