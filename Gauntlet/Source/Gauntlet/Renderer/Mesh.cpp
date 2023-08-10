#include "GauntletPCH.h"
#include "Mesh.h"

#include "Texture.h"
#include "RendererAPI.h"

#include "Gauntlet/Renderer/Renderer.h"
#include "Gauntlet/Core/JobSystem.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Gauntlet
{
Ref<Mesh> Mesh::Create(const std::string& InFilePath)
{
    return Ref<Mesh>(new Mesh(InFilePath));
}

Mesh::Mesh(const std::vector<Vertex>& InVertices, const std::vector<uint32_t>& InIndices)
{
    LOG_INFO("Creating cube!");
    JobSystem::Submit(
        [this, InVertices, InIndices]
        {
            BufferInfo VertexBufferInfo = {};
            VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
            VertexBufferInfo.Layout     = {{EShaderDataType::Vec3, "InPosition"}};
            VertexBufferInfo.Count      = InVertices.size();

            Ref<VertexBuffer> VertexBuffer(VertexBuffer::Create(VertexBufferInfo));
            VertexBuffer->SetData(InVertices.data(), InVertices.size() * sizeof(InVertices[0]));
            m_VertexBuffers.emplace_back(VertexBuffer);

            BufferInfo IndexBufferInfo = {};
            IndexBufferInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;
            IndexBufferInfo.Count      = InIndices.size();
            IndexBufferInfo.Data       = (void*)InIndices.data();
            IndexBufferInfo.Size       = InIndices.size() * sizeof(InIndices[0]);

            m_IndexBuffers.emplace_back(IndexBuffer::Create(IndexBufferInfo));
        });
}

Mesh::Mesh(const std::string& InMeshPath) : m_Directory(InMeshPath)
{
    LOG_INFO("Creating mesh!");
    JobSystem::Submit(
        [this]
        {
            LoadMesh(m_Directory);

            BufferInfo VertexBufferInfo = {};
            VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;

            VertexBufferInfo.Layout = {
                {EShaderDataType::Vec3, "InPosition"},  //
                {EShaderDataType::Vec3, "InNormal"},    //
                {EShaderDataType::Vec4, "InColor"},     //
                {EShaderDataType::Vec2, "InTexCoord"},  //
                {EShaderDataType::Vec3, "InTangent"},   //
                {EShaderDataType::Vec3, "InBitangent"}  //
            };

            BufferInfo IndexBufferInfo = {};
            IndexBufferInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;

            // MeshesData loaded, let's populate index/vertex buffers
            for (auto& OneMeshData : m_MeshesData)
            {
                VertexBufferInfo.Count = OneMeshData.Vertices.size();
                Ref<VertexBuffer> VertexBuffer(VertexBuffer::Create(VertexBufferInfo));
                VertexBuffer->SetData(OneMeshData.Vertices.data(), OneMeshData.Vertices.size() * sizeof(OneMeshData.Vertices[0]));
                m_VertexBuffers.emplace_back(VertexBuffer);

                IndexBufferInfo.Count = OneMeshData.Indices.size();
                IndexBufferInfo.Data  = OneMeshData.Indices.data();
                IndexBufferInfo.Size  = OneMeshData.Indices.size() * sizeof(OneMeshData.Indices[0]);

                m_IndexBuffers.emplace_back(IndexBuffer::Create(IndexBufferInfo));
            }
        });
}

void Mesh::LoadMesh(const std::string& InMeshPath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(InMeshPath.data(), aiProcess_Triangulate | aiProcess_GenNormals |
                                                                    aiProcess_PreTransformVertices | aiProcess_OptimizeMeshes);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("ERROR::ASSIMP:: %s", importer.GetErrorString());
        GNT_ASSERT(false, "Failed to load model!");
        return;
    }

    m_Directory = std::string(InMeshPath.substr(0, InMeshPath.find_last_of('/'))) + std::string("/");
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

        Vertex.Normal = glm::vec3(0.0f);
        if (mesh->HasNormals()) Vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        Vertex.Color = glm::vec4(1.0f);
        if (mesh->HasVertexColors(0))
            Vertex.Color = glm::vec4((float)mesh->mColors[0][i].r, (float)mesh->mColors[0][i].g, (float)mesh->mColors[0][i].b,
                                     (float)mesh->mColors[0][i].a);

        Vertex.TexCoord = glm::vec2(0.0f);
        if (mesh->HasTextureCoords(0)) Vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        Vertex.Tangent = glm::vec3(0.0f);
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
    std::vector<MeshTexture> DiffuseTextures;
    std::vector<MeshTexture> NormalMapTextures;
    std::vector<MeshTexture> EmissiveTextures;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto DiffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
        if (DiffuseMaps.empty()) DiffuseMaps = LoadMaterialTextures(material, aiTextureType_BASE_COLOR);

        DiffuseTextures.insert(DiffuseTextures.end(), DiffuseMaps.begin(), DiffuseMaps.end());

        const auto NormalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS);
        NormalMapTextures.insert(NormalMapTextures.end(), NormalMaps.begin(), NormalMaps.end());

        auto EmissiveMaps = LoadMaterialTextures(material, aiTextureType_EMISSIVE);
        if (EmissiveMaps.empty()) EmissiveMaps = LoadMaterialTextures(material, aiTextureType_SHININESS);
        if (EmissiveMaps.empty()) EmissiveMaps = LoadMaterialTextures(material, aiTextureType_EMISSION_COLOR);
        EmissiveTextures.insert(EmissiveTextures.end(), EmissiveMaps.begin(), EmissiveMaps.end());
    }

    return MeshData(Vertices, Indices, DiffuseTextures, NormalMapTextures, EmissiveTextures);
}

std::vector<Mesh::MeshTexture> Mesh::LoadMaterialTextures(aiMaterial* mat, aiTextureType type)
{
    std::vector<MeshTexture> Textures;
    const uint32_t TextureCount = mat->GetTextureCount(type);
    for (uint32_t i = 0; i < TextureCount; ++i)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool bIsLoaded{false};
        for (uint32_t j = 0; j < m_LoadedTextures.size(); ++j)
        {
            // Already loaded, let's get it.
            if (std::strcmp(m_LoadedTextures[j].FilePath.data(), str.C_Str()) == 0)
            {
                Textures.push_back(m_LoadedTextures[j]);
                bIsLoaded = true;
                break;
            }
        }

        // Hasn't loaded, so lets load it.
        if (!bIsLoaded)
        {
            const std::string LocalTexturePath(str.C_Str());
            const auto TexturePath = m_Directory + LocalTexturePath;
            MeshTexture texture    = {};
            texture.Texture        = Ref<Texture2D>(Texture2D::Create(TexturePath));
            texture.FilePath       = LocalTexturePath;

            Textures.emplace_back(texture);
            m_LoadedTextures.push_back(texture);
        }
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

    for (auto& NormalMapTexture : m_LoadedTextures)
        NormalMapTexture.Texture->Destroy();
}

}  // namespace Gauntlet