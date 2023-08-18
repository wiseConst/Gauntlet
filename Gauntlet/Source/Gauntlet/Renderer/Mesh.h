#pragma once

#include "Gauntlet/Core/Core.h"
#include "Gauntlet/Renderer/Buffer.h"

#include "Gauntlet/Renderer/CoreRendererStructs.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum aiTextureType;

namespace Gauntlet
{

class Material;
class Texture2D;

class Mesh final
{
  public:
    Mesh()          = default;
    virtual ~Mesh() = default;

    void Destroy();

    FORCEINLINE const auto& GetVertexBuffers() const { return m_VertexBuffers; }
    FORCEINLINE const auto& GetIndexBuffers() const { return m_IndexBuffers; }

    FORCEINLINE const uint32_t GetSubmeshCount() const { return m_MeshesData.size(); }
    FORCEINLINE const auto& GetSubmeshName(const uint32_t InMeshIndex) { return m_MeshesData[InMeshIndex].Name; }

    FORCEINLINE const Ref<Gauntlet::Material>& GetMaterial(const uint32_t InMeshIndex) { return m_MeshesData[InMeshIndex].Material; }

    static Ref<Mesh> Create(const std::string& InModelPath);
    static Ref<Mesh> CreateCube();

  private:
    // Actual submesh
    struct MeshData
    {
        MeshData(const std::string& InName, std::vector<MeshVertex>& InVertices, const std::vector<uint32_t>& InIndices,
                 const Ref<Gauntlet::Material>& InMaterial)
            : Name(InName), Vertices(InVertices), Indices(InIndices), Material(InMaterial)
        {
        }

        std::vector<MeshVertex> Vertices;
        std::vector<uint32_t> Indices;
        Ref<Gauntlet::Material> Material;
        std::string Name;
    };

    std::string m_Directory;
    std::vector<MeshData> m_MeshesData;  // Is it submeshes?

    // Optimization to prevent loading the same textures.
    std::unordered_map<std::string, Ref<Texture2D>> m_LoadedTextures;

    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    std::vector<Ref<IndexBuffer>> m_IndexBuffers;

    Mesh(const std::vector<Vertex>& InVertices, const std::vector<uint32_t>& InIndices);
    Mesh(const std::string& InMeshPath);

    void LoadMesh(const std::string& InMeshPath);

    void ProcessNode(aiNode* node, const aiScene* scene);
    MeshData ProcessMeshData(aiMesh* mesh, const aiScene* scene);
    std::vector<Ref<Texture2D>> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);
};

}  // namespace Gauntlet