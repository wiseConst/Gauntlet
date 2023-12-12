#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"

#include "Gauntlet/Renderer/CoreRendererTypes.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiNodeAnim;
enum aiTextureType;

namespace Gauntlet
{

class Material;
class Texture2D;

struct KeyPosition
{
    glm::vec3 Position;
    float Timetamp;
};

struct KeyRotation
{
    glm::quat Orientation;
    float Timestamp;
};

struct KeyScale
{
    glm::vec3 Scale;
    float Timestamp;
};

class Bone final : private Unmovable, private Uncopyable
{
  public:
    Bone(const std::string& name, int32_t ID, const aiNodeAnim* channel);

    /*
    interpolates  bones /w positions,rotations & scaling keys based on the current time of
  the animation and prepares the local transformation matrix by combining all keys
  tranformations
  */
    void Update(float animationTime);

    /* Gets the current index on mKeyPositions to interpolate to based on
    the current animation time*/
    int32_t GetPositionIndex(float animationTime)
    {
        for (int32_t index = 0; index < m_NumPositions - 1; ++index)
        {
            if (animationTime < m_Positions[index + 1].Timetamp) return index;
        }
        GNT_ASSERT(0);
    }

    /* Gets the current index on mKeyRotations to interpolate to based on the
    current animation time*/
    int32_t GetRotationIndex(float animationTime)
    {
        for (int32_t index = 0; index < m_NumRotations - 1; ++index)
        {
            if (animationTime < m_Rotations[index + 1].Timestamp) return index;
        }
        GNT_ASSERT(0);
    }

    /* Gets the current index on mKeyScalings to interpolate to based on the
    current animation time */
    int32_t GetScaleIndex(float animationTime)
    {
        for (int32_t index = 0; index < m_NumScalings - 1; ++index)
        {
            if (animationTime < m_Scales[index + 1].Timestamp) return index;
        }
        GNT_ASSERT(0);
    }

    FORCEINLINE glm::mat4 GetLocalTransform() { return m_LocalTransform; }
    FORCEINLINE std::string GetName() const { return m_Name; }
    FORCEINLINE int32_t GetID() { return m_ID; }

  private:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int32_t m_NumPositions = 0;
    int32_t m_NumRotations = 0;
    int32_t m_NumScalings  = 0;

    glm::mat4 m_LocalTransform = glm::mat4(1.0f);
    std::string m_Name;
    int32_t m_ID = 0;

    /* Gets normalized value for Lerp & Slerp */
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float scaleFactor  = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff   = nextTimeStamp - lastTimeStamp;
        scaleFactor        = midWayLength / framesDiff;
        return scaleFactor;
    }

    /*figures out which position keys to interpolate b/w and performs the interpolation
    and returns the translation matrix*/
    glm::mat4 InterpolatePosition(float animationTime)
    {
        if (1 == m_NumPositions) return glm::translate(glm::mat4(1.0f), m_Positions[0].Position);

        int32_t p0Index         = GetPositionIndex(animationTime);
        int32_t p1Index         = p0Index + 1;
        float scaleFactor       = GetScaleFactor(m_Positions[p0Index].Timetamp, m_Positions[p1Index].Timetamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].Position, m_Positions[p1Index].Position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    /*figures out which rotations keys to interpolate b/w and performs the interpolation
    and returns the rotation matrix*/
    glm::mat4 InterpolateRotation(float animationTime)
    {
        if (1 == m_NumRotations)
        {
            auto rotation = glm::normalize(m_Rotations[0].Orientation);
            return glm::toMat4(rotation);
        }

        int32_t p0Index         = GetRotationIndex(animationTime);
        int32_t p1Index         = p0Index + 1;
        float scaleFactor       = GetScaleFactor(m_Rotations[p0Index].Timestamp, m_Rotations[p1Index].Timestamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].Orientation, m_Rotations[p1Index].Orientation, scaleFactor);
        finalRotation           = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    /*figures out which scaling keys to interpolate b/w and performs the interpolation
    and returns the scale matrix*/
    glm::mat4 InterpolateScaling(float animationTime)
    {
        if (1 == m_NumScalings) return glm::scale(glm::mat4(1.0f), m_Scales[0].Scale);

        int32_t p0Index      = GetScaleIndex(animationTime);
        int32_t p1Index      = p0Index + 1;
        float scaleFactor    = GetScaleFactor(m_Scales[p0Index].Timestamp, m_Scales[p1Index].Timestamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].Scale, m_Scales[p1Index].Scale, scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }
};

class Submesh final
{
  public:
    Submesh(const std::string& InName, std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices,
            const Ref<Gauntlet::Material>& material)
        : Name(InName), Vertices(vertices), Indices(indices), Material(material)
    {
    }

    Submesh(const std::string& InName, std::vector<AnimatedVertex>& vertices, const std::vector<uint32_t>& indices,
            const Ref<Gauntlet::Material>& material)
        : Name(InName), AnimatedVertices(vertices), Indices(indices), Material(material)
    {
    }

    Submesh() = default;

    //   private:
    std::vector<MeshVertex> Vertices;
    std::vector<AnimatedVertex> AnimatedVertices;
    std::vector<uint32_t> Indices;
    Ref<Gauntlet::Material> Material;
    std::string Name;
};

struct BoneInfo
{
    int32_t ID;        // index in finalBoneMatrices
    glm::mat4 Offset;  // offset matrix transforms vertex from model space to bone space
};

class Animation;

class Mesh final
{
  public:
    Mesh() = default;
    ~Mesh();

    FORCEINLINE const auto& GetVertexBuffers() const { return m_VertexBuffers; }
    FORCEINLINE const auto& GetIndexBuffers() const { return m_IndexBuffers; }

    FORCEINLINE const uint32_t GetSubmeshCount() const { return static_cast<uint32_t>(m_Submeshes.size()); }
    FORCEINLINE const auto& GetSubmeshName(const uint32_t meshIndex) { return m_Submeshes[meshIndex].Name; }
    FORCEINLINE const auto& GetMeshNameWithDirectory() { return m_Name; }

    FORCEINLINE const Ref<Gauntlet::Material>& GetMaterial(const uint32_t meshIndex) { return m_Submeshes[meshIndex].Material; }
    FORCEINLINE bool IsAnimated() const { return m_bIsAnimated; }
    FORCEINLINE Ref<Animation>& GetAnimation() { return m_Animation; }

    static Ref<Mesh> Create(const std::string& modelPath);

  private:
    std::string m_Name{"None"};
    std::string m_Directory;
    std::vector<Submesh> m_Submeshes;

    bool m_bIsAnimated         = false;
    Ref<Animation> m_Animation = nullptr;
    std::map<std::string, BoneInfo> m_BoneMap;

    // Optimization to prevent loading the same textures.
    std::unordered_map<std::string, Ref<Texture2D>> m_LoadedTextures;

    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    std::vector<Ref<IndexBuffer>> m_IndexBuffers;

    Mesh(const std::string& meshPath);
    void Destroy();

    void LoadMesh(const std::string& meshPath);
    void LoadAnimation(const aiScene* scene);

    void ProcessNode(aiNode* node, const aiScene* scene);
    Submesh ProcessSubmesh(aiMesh* mesh, const aiScene* scene);

    template <typename VertexType> void OptimizeMesh(Submesh& submesh);

    Submesh ProcessAnimatedSubmesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Ref<Texture2D>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);

    void ExtractBoneWeightForVertices(std::vector<AnimatedVertex>& vertices, aiMesh* mesh, const aiScene* scene);

    void SetVertexBoneData(AnimatedVertex& vertex, int32_t boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.BoneIDs[i] < 0)
            {
                vertex.Weights[i] = weight;
                vertex.BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void SetVertexBoneDataToDefault(AnimatedVertex& vertex)
    {
        for (uint32_t i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            vertex.BoneIDs[i] = -1;
            vertex.Weights[i] = 0.0f;
        }
    }
};

}  // namespace Gauntlet