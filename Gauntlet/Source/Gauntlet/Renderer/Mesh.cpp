#include "GauntletPCH.h"
#include "Mesh.h"

#include "Texture.h"
#include "Material.h"
#include "RendererAPI.h"

#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Core/JobSystem.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Gauntlet
{
Ref<Mesh> Mesh::Create(const std::string& filePath)
{
    return Ref<Mesh>(new Mesh(filePath));
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    JobSystem::Submit(
        [this, vertices, indices]
        {
            BufferInfo VertexBufferInfo = {};
            VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
            VertexBufferInfo.Layout     = {{EShaderDataType::Vec3, "InPosition"}};
            VertexBufferInfo.Count      = vertices.size();

            Ref<Gauntlet::VertexBuffer> VertexBuffer(Gauntlet::VertexBuffer::Create(VertexBufferInfo));
            VertexBuffer->SetData(vertices.data(), vertices.size() * sizeof(vertices[0]));
            m_VertexBuffers.emplace_back(VertexBuffer);

            BufferInfo IndexBufferInfo = {};
            IndexBufferInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;
            IndexBufferInfo.Count      = indices.size();
            IndexBufferInfo.Data       = (void*)indices.data();
            IndexBufferInfo.Size       = indices.size() * sizeof(indices[0]);

            m_IndexBuffers.emplace_back(Gauntlet::IndexBuffer::Create(IndexBufferInfo));
        });
}

Mesh::Mesh(const std::string& meshPath)
{
    JobSystem::Submit(
        [this, meshPath]
        {
            LoadMesh(meshPath);

            BufferInfo VertexBufferInfo = {};
            VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
            VertexBufferInfo.Layout     = Renderer::GetStorageData().MeshVertexBufferLayout;

            BufferInfo IndexBufferInfo = {};
            IndexBufferInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;

            // MeshesData loaded, let's populate index/vertex buffers
            for (auto& OneMeshData : m_MeshesData)
            {
                VertexBufferInfo.Count = OneMeshData.Vertices.size();
                Ref<Gauntlet::VertexBuffer> VertexBuffer(Gauntlet::VertexBuffer::Create(VertexBufferInfo));
                VertexBuffer->SetData(OneMeshData.Vertices.data(), OneMeshData.Vertices.size() * sizeof(OneMeshData.Vertices[0]));
                m_VertexBuffers.emplace_back(VertexBuffer);

                IndexBufferInfo.Count = OneMeshData.Indices.size();
                IndexBufferInfo.Data  = OneMeshData.Indices.data();
                IndexBufferInfo.Size  = OneMeshData.Indices.size() * sizeof(OneMeshData.Indices[0]);

                m_IndexBuffers.emplace_back(Gauntlet::IndexBuffer::Create(IndexBufferInfo));
            }
        });
}

void Mesh::LoadMesh(const std::string& meshPath)
{
    Assimp::Importer importer;
    const auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
                             aiProcess_OptimizeMeshes | aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
                             aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInstances |
                             aiProcess_ValidateDataStructure | aiProcess_FindDegenerates | aiProcess_FindInvalidData;
    const aiScene* scene = importer.ReadFile(meshPath.data(), importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("ERROR::ASSIMP:: %s", importer.GetErrorString());
        GNT_ASSERT(false, "Failed to load model!");
        return;
    }

    m_Directory = std::string(meshPath.substr(0, meshPath.find_last_of('/'))) + std::string("/");
    {
        uint32_t i = 0;  // Models
        for (i = 2; i < meshPath.size(); ++i)
        {
            if (meshPath[i - 2] == 'l' && meshPath[i - 1] == 's' && meshPath[i] == '/') break;
        }
        m_Name = std::string(meshPath.substr(i + 1, meshPath.size() - i + 1));
    }

    LOG_TRACE("Loading mesh: %s...", m_Name.data());
    ProcessNode(scene->mRootNode, scene);
}

void Mesh::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_MeshesData.push_back(ProcessMeshData(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

Mesh::MeshData Mesh::ProcessMeshData(aiMesh* mesh, const aiScene* scene)
{
    // Vertices
    std::vector<MeshVertex> Vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        MeshVertex Vertex;
        Vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        Vertex.Color = glm::vec4(1.0f);
        if (mesh->HasVertexColors(0))
            Vertex.Color = glm::vec4((float)mesh->mColors[0][i].r, (float)mesh->mColors[0][i].g, (float)mesh->mColors[0][i].b,
                                     (float)mesh->mColors[0][i].a);

        Vertex.Normal = glm::vec3(0.0f);
        if (mesh->HasNormals()) Vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        Vertex.TexCoord = glm::vec2(0.0f);
        if (mesh->HasTextureCoords(0)) Vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        Vertex.Tangent   = glm::vec3(0.0f);
        Vertex.Bitangent = glm::vec3(0.0f);
        if (mesh->HasTangentsAndBitangents())
        {
            Vertex.Tangent   = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            Vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }

        Vertices.push_back(Vertex);
    }

    // Indices
    std::vector<uint32_t> Indices;
    GNT_ASSERT(mesh->HasFaces(), "Your mesh doesn't have faces(any primitves that can be drawn).");
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            Indices.push_back(face.mIndices[j]);
    }

    // Materials
    std::vector<Ref<Texture2D>> DiffuseTextures;
    std::vector<Ref<Texture2D>> NormalMapTextures;
    std::vector<Ref<Texture2D>> EmissiveTextures;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto DiffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
        if (DiffuseMaps.empty()) DiffuseMaps = LoadMaterialTextures(material, aiTextureType_BASE_COLOR);

        DiffuseTextures.insert(DiffuseTextures.end(), DiffuseMaps.begin(), DiffuseMaps.end());

        const auto NormalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS);
        NormalMapTextures.insert(NormalMapTextures.end(), NormalMaps.begin(), NormalMaps.end());

        // Testing only
        if (DiffuseTextures.empty())
        {
            auto DiffuseMaps = LoadMaterialTextures(material, aiTextureType_EMISSIVE);
            if (DiffuseMaps.empty()) DiffuseMaps = LoadMaterialTextures(material, aiTextureType_SHININESS);
            if (DiffuseMaps.empty()) DiffuseMaps = LoadMaterialTextures(material, aiTextureType_EMISSION_COLOR);
            DiffuseTextures.insert(DiffuseTextures.end(), DiffuseMaps.begin(), DiffuseMaps.end());
        }

        auto EmissiveMaps = LoadMaterialTextures(material, aiTextureType_EMISSIVE);
        if (EmissiveMaps.empty()) EmissiveMaps = LoadMaterialTextures(material, aiTextureType_SHININESS);
        if (EmissiveMaps.empty()) EmissiveMaps = LoadMaterialTextures(material, aiTextureType_EMISSION_COLOR);
        EmissiveTextures.insert(EmissiveTextures.end(), EmissiveMaps.begin(), EmissiveMaps.end());
    }

    Ref<Gauntlet::Material> Material = Material::Create();
    Material->SetDiffuseTextures(DiffuseTextures);
    Material->SetNormalMapTextures(NormalMapTextures);

    // Currently unused
    Material->SetEmissiveTextures(EmissiveTextures);

    Material->Invalidate();

    return MeshData(mesh->mName.C_Str(), Vertices, Indices, Material);
}

std::vector<Ref<Texture2D>> Mesh::LoadMaterialTextures(aiMaterial* mat, aiTextureType type)
{
    std::vector<Ref<Texture2D>> Textures;
    const uint32_t TextureCount = mat->GetTextureCount(type);
    for (uint32_t i = 0; i < TextureCount; ++i)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool bIsLoaded = m_LoadedTextures.contains(str.C_Str());
        if (bIsLoaded)
        {
            Textures.push_back(m_LoadedTextures[str.C_Str()]);
            continue;
        }

        // Hasn't loaded, so lets load it.
        const std::string LocalTexturePath(str.C_Str());
        const auto TexturePath           = m_Directory + LocalTexturePath;
        TextureSpecification textureSpec = {};
        textureSpec.CreateTextureID      = true;
        textureSpec.GenerateMips         = true;
        textureSpec.Filter               = ETextureFilter::LINEAR;
        textureSpec.Wrap                 = ETextureWrap::REPEAT;
        Ref<Texture2D> texture =
            Ref<Texture2D>(Texture2D::Create(TexturePath, textureSpec));  // Temporary create TextureID's for Mesh Textures

        LOG_TRACE("Loaded texture: %s", TexturePath.data());

        Textures.emplace_back(texture);
        m_LoadedTextures[LocalTexturePath] = texture;
    }

    return Textures;
}

Ref<Mesh> Mesh::CreateCube()
{
    const std::vector<Vertex> Positions = {
        Vertex({-1, -1, -1}), Vertex({1, -1, -1}), Vertex({1, 1, -1}), Vertex({-1, 1, -1}),
        Vertex({-1, -1, 1}),  Vertex({1, -1, 1}),  Vertex({1, 1, 1}),  Vertex({-1, 1, 1}),
    };

    const std::vector<uint32_t> Indices = {
        0, 1, 3, 3, 1, 2,  //
        1, 5, 2, 2, 5, 6,  //
        5, 4, 6, 6, 4, 7,  //
        4, 0, 7, 7, 0, 3,  //
        3, 2, 7, 7, 2, 6,  //
        4, 5, 0, 0, 5, 1   //
    };

    return Ref<Mesh>(new Mesh(Positions, Indices));
}

void Mesh::Destroy()
{
    for (auto& VertexBuffer : m_VertexBuffers)
        VertexBuffer->Destroy();

    for (auto& IndexBuffer : m_IndexBuffers)
        IndexBuffer->Destroy();

    for (auto& submesh : m_MeshesData)
        submesh.Material->Destroy();

    for (auto& LoadedTexture : m_LoadedTextures)
        LoadedTexture.second->Destroy();
}

}  // namespace Gauntlet