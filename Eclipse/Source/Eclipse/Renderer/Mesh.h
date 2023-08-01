#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/Buffer.h"

#include "Eclipse/Renderer/CoreRendererStructs.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

namespace Eclipse
{

class Texture2D;

class Mesh final
{
  public:
    Mesh()          = default;
    virtual ~Mesh() = default;

    void Destroy();

    FORCEINLINE const auto& GetVertexBuffers() const { return m_VertexBuffers; }
    FORCEINLINE const auto& GetIndexBuffers() const { return m_IndexBuffers; }

    FORCEINLINE bool HasDiffuseTexture(const uint32_t MeshIndex) const { return m_MeshesData[MeshIndex].DiffuseTextures.size() > 0; }
    FORCEINLINE bool HasNormalMapTexture(const uint32_t MeshIndex) const { return m_MeshesData[MeshIndex].NormalMapTextures.size() > 0; }

    FORCEINLINE const auto& GetDiffuseTexture(const uint32_t MeshIndex, const uint32_t DiffuseTextureIndex = 0) const
    {
        return m_MeshesData[MeshIndex].DiffuseTextures[DiffuseTextureIndex].Texture;
    }

    FORCEINLINE const auto& GetNormalMapTexture(const uint32_t MeshIndex, const uint32_t NormalMapTextureIndex = 0) const
    {
        return m_MeshesData[MeshIndex].NormalMapTextures[NormalMapTextureIndex].Texture;
    }

    static Ref<Mesh> Create(const std::string& InModelPath);
    static Ref<Mesh> CreateCube();

  private:
    struct MeshTexture
    {
        Ref<Texture2D> Texture;
        std::string FilePath;
    };

    struct MeshData
    {
        MeshData(const std::vector<MeshVertex>& InVertices, const std::vector<uint32_t>& InIndices,
                 const std::vector<MeshTexture>& InDiffuseTextures, const std::vector<MeshTexture>& InNormalMapTextures)
            : Vertices(InVertices), Indices(InIndices), DiffuseTextures(InDiffuseTextures), NormalMapTextures(InNormalMapTextures)
        {
        }

        std::vector<MeshVertex> Vertices;
        std::vector<uint32_t> Indices;
        std::vector<MeshTexture> DiffuseTextures;
        std::vector<MeshTexture> NormalMapTextures;
    };

    std::string m_Directory;
    std::vector<MeshData> m_MeshesData;  // Is it submeshes?

    // Optimization to prevent loading the same textures.
    std::vector<MeshTexture> m_LoadedTextures;

    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    std::vector<Ref<IndexBuffer>> m_IndexBuffers;

    Mesh(const std::vector<Vertex>& InVertices, const std::vector<uint32_t>& InIndices);
    Mesh(const std::string& InMeshPath);

    void LoadMesh(const std::string& InMeshPath);

    void ProcessNode(aiNode* node, const aiScene* scene);
    MeshData ProcessMeshData(aiMesh* mesh, const aiScene* scene);
    std::vector<MeshTexture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);
};

}  // namespace Eclipse