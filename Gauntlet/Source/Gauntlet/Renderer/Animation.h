#pragma once

#include "Gauntlet/Core/Core.h"
#include "Mesh.h"
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Gauntlet
{

struct AssimpNodeData
{
    glm::mat4 Transformation;
    std::string Name;
    int32_t ChildrenCount;
    std::vector<AssimpNodeData> Children;
};

class Animation final
{
  public:
    Animation() = default;

    Animation(const aiScene* scene)
    {
        GNT_ASSERT(scene && scene->mRootNode);
        auto animation = scene->mAnimations[0];

        LOG_WARN("Loading animation: %s", animation->mName.C_Str());

        m_Duration       = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
    }

    ~Animation() {}

    Bone* FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(), [&](const Bone& Bone) { return Bone.GetName() == name; });
        if (iter == m_Bones.end())
            return nullptr;
        else
            return &(*iter);
    }

    inline float GetTicksPerSecond() { return m_TicksPerSecond; }

    inline float GetDuration() { return m_Duration; }

    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }

    FORCEINLINE const std::map<std::string, BoneInfo>& GetBoneIDMap() { return m_BoneMap; }

  private:
    float m_Duration         = 0.0f;
    int32_t m_TicksPerSecond = 0;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneMap;

    friend class Mesh;

    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        GNT_ASSERT(src);

        dest.Name = src->mName.data;
        for (uint32_t i = 0; i < 4; ++i)
        {
            for (uint32_t j = 0; j < 4; ++j)
            {
                dest.Transformation[i][j] = src->mTransformation[i][j];
            }
        }
        dest.ChildrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.Children.push_back(newData);
        }
    }
};

class Animator final
{
  public:
    Animator(Animation* animation)
    {
        m_CurrentTime      = 0.0;
        m_CurrentAnimation = animation;

        m_FinalBoneMatrices.reserve(100);

        for (int i = 0; i < 100; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    void UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        if (m_CurrentAnimation)
        {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }

    void PlayAnimation(Animation* pAnimation)
    {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime      = 0.0f;
    }

    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName    = node->Name;
        glm::mat4 nodeTransform = node->Transformation;

        Bone* bone = m_CurrentAnimation->FindBone(nodeName);

        if (bone)
        {
            bone->Update(m_CurrentTime);
            nodeTransform = bone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end())
        {
            int index                  = boneInfoMap[nodeName].ID;
            glm::mat4 offset           = boneInfoMap[nodeName].Offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }

        for (int i = 0; i < node->ChildrenCount; i++)
            CalculateBoneTransform(&node->Children[i], globalTransformation);
    }

    std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }

  private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;
};

}  // namespace Gauntlet