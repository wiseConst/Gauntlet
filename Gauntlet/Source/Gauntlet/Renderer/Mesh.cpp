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

#include "Animation.h"

namespace Gauntlet
{

Bone::Bone(const std::string& name, int32_t ID, const aiNodeAnim* channel) : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
{
    m_NumPositions = channel->mNumPositionKeys;

    for (int32_t positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        KeyPosition data;
        data.Position = glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z);
        data.Timetamp = channel->mPositionKeys[positionIndex].mTime;
        m_Positions.push_back(data);
    }

    m_NumRotations = channel->mNumRotationKeys;
    for (int32_t rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        KeyRotation data;
        data.Orientation = glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z);
        data.Timestamp   = channel->mRotationKeys[rotationIndex].mTime;
        m_Rotations.push_back(data);
    }

    m_NumScalings = channel->mNumScalingKeys;
    for (int32_t keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
    {
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        KeyScale data;
        data.Scale     = glm::vec3(scale.x, scale.y, scale.z);
        data.Timestamp = channel->mScalingKeys[keyIndex].mTime;
        m_Scales.push_back(data);
    }
}

void Bone::Update(float animationTime)
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation    = InterpolateRotation(animationTime);
    glm::mat4 scale       = InterpolateScaling(animationTime);
    m_LocalTransform      = translation * rotation * scale;
}

Ref<Mesh> Mesh::Create(const std::string& filePath)
{
    return Ref<Mesh>(new Mesh(filePath));
}

Mesh::Mesh(const std::string& meshPath)
{
    JobSystem::Submit(
        [this, meshPath]
        {
            LoadMesh(meshPath);

            BufferInfo vbInfo = {};
            vbInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
            vbInfo.Layout =
                m_bIsAnimated ? Renderer::GetStorageData().AnimatedVertexBufferLayout : Renderer::GetStorageData().MeshVertexBufferLayout;

            BufferInfo ibInfo = {};
            ibInfo.Usage      = EBufferUsageFlags::INDEX_BUFFER;

            for (auto& submesh : m_Submeshes)
            {
                if (m_bIsAnimated)
                    vbInfo.Count = submesh.AnimatedVertices.size();
                else
                    vbInfo.Count = submesh.Vertices.size();

                Ref<Gauntlet::VertexBuffer> vertexBuffer = Gauntlet::VertexBuffer::Create(vbInfo);

                if (m_bIsAnimated)
                    vertexBuffer->SetData(submesh.AnimatedVertices.data(),
                                          submesh.AnimatedVertices.size() * sizeof(submesh.AnimatedVertices[0]));
                else
                    vertexBuffer->SetData(submesh.Vertices.data(), submesh.Vertices.size() * sizeof(submesh.Vertices[0]));

                m_VertexBuffers.emplace_back(vertexBuffer);

                ibInfo.Count = submesh.Indices.size();
                ibInfo.Data  = submesh.Indices.data();
                ibInfo.Size  = submesh.Indices.size() * sizeof(submesh.Indices[0]);

                m_IndexBuffers.emplace_back(Gauntlet::IndexBuffer::Create(ibInfo));
            }
        });
}

void Mesh::LoadAnimation(const aiScene* scene)
{
    m_Animation       = MakeRef<Animation>(scene);
    aiAnimation* anim = scene->mAnimations[0];
    LOG_WARN("Loading animation: %s", anim->mName.C_Str());

    m_Animation->m_Duration       = anim->mDuration;
    m_Animation->m_TicksPerSecond = anim->mTicksPerSecond;
}

void Mesh::LoadMesh(const std::string& meshPath)
{
    Assimp::Importer importer;
    const auto importFlags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
                             aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices |
                             aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInstances | aiProcess_ValidateDataStructure |
                             aiProcess_FindDegenerates | aiProcess_FindInvalidData;
    const aiScene* scene = importer.ReadFile(meshPath.data(), importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("ERROR::ASSIMP:: %s", importer.GetErrorString());
        return;
    }

    m_Directory = std::string(meshPath.substr(0, meshPath.find_last_of('/'))) + std::string("/");
    {
        size_t pos = meshPath.find("Models/");
        GNT_ASSERT(pos != std::string::npos);

        m_Name = meshPath.substr(pos + 7, meshPath.size() - pos);
    }

    // m_bIsAnimated = scene->HasAnimations();
    if (m_bIsAnimated) LoadAnimation(scene);

    LOG_TRACE("Loading mesh: %s...", m_Name.data());
    ProcessNode(scene->mRootNode, scene);
}

void Mesh::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_Submeshes.push_back(m_bIsAnimated ? ProcessAnimatedSubmesh(mesh, scene) : ProcessSubmesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

Submesh Mesh::ProcessAnimatedSubmesh(aiMesh* mesh, const aiScene* scene)
{
    // Vertices
    std::vector<AnimatedVertex> Vertices;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        AnimatedVertex vertex;
        SetVertexBoneDataToDefault(vertex);

        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal   = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        vertex.Tangent = glm::vec3(0.0f);
        if (mesh->HasTangentsAndBitangents()) vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

        Vertices.push_back(vertex);
    }
    ExtractBoneWeightForVertices(Vertices, mesh, scene);

    // Indices
    std::vector<uint32_t> Indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; ++j)
            Indices.push_back(face.mIndices[j]);
    }

    // Materials
    std::vector<Ref<Texture2D>> albedoTextures;
    std::vector<Ref<Texture2D>> normalTextures;
    std::vector<Ref<Texture2D>> metallicTextures;
    std::vector<Ref<Texture2D>> roughnessTextures;
    std::vector<Ref<Texture2D>> aoTextures;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        const auto baseColor = LoadMaterialTextures(material, aiTextureType_BASE_COLOR);

        const auto albedos = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
        albedoTextures.insert(albedoTextures.end(), albedos.begin(), albedos.end());

        const auto normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS);
        normalTextures.insert(normalTextures.end(), normalMaps.begin(), normalMaps.end());

        const auto metallic = LoadMaterialTextures(material, aiTextureType_METALNESS);
        metallicTextures.insert(metallicTextures.end(), metallic.begin(), metallic.end());

        const auto rougnhess = LoadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS);
        roughnessTextures.insert(roughnessTextures.end(), rougnhess.begin(), rougnhess.end());

        const auto ao = LoadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION);
        aoTextures.insert(aoTextures.end(), ao.begin(), ao.end());
    }

    Ref<Gauntlet::Material> Material = Material::Create();
    Material->m_AlbedoTextures       = albedoTextures;
    Material->m_NormalTextures       = normalTextures;
    Material->m_MetallicTextures     = metallicTextures;
    Material->m_RougnessTextures     = roughnessTextures;
    Material->m_AOTextures           = aoTextures;
    Material->Invalidate();

    return Submesh(mesh->mName.C_Str(), Vertices, Indices, Material);
}

void Mesh::ExtractBoneWeightForVertices(std::vector<AnimatedVertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int32_t boneID       = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (m_BoneMap.find(boneName) == m_BoneMap.end())
        {
            LOG_DEBUG("BoneInfoMap adding: %s", boneName.c_str());

            BoneInfo newBoneInfo;
            newBoneInfo.ID = m_BoneMap.size();
            for (uint32_t i = 0; i < 4; ++i)
            {
                for (uint32_t j = 0; j < 4; ++j)
                {
                    newBoneInfo.Offset[i][j] = mesh->mBones[boneIndex]->mOffsetMatrix[i][j];
                }
            }
            m_BoneMap[boneName] = newBoneInfo;
            boneID              = newBoneInfo.ID;
        }
        else
            boneID = m_BoneMap[boneName].ID;

        GNT_ASSERT(boneID != -1);
        const auto weights       = mesh->mBones[boneIndex]->mWeights;
        const int32_t numWeights = mesh->mBones[boneIndex]->mNumWeights;
        for (int32_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            const int32_t vertexID = weights[weightIndex].mVertexId;
            float weight           = weights[weightIndex].mWeight;
            GNT_ASSERT(vertexID <= vertices.size());

            SetVertexBoneData(vertices[vertexID], boneID, weight);
        }
    }
}

Submesh Mesh::ProcessSubmesh(aiMesh* mesh, const aiScene* scene)
{
    // Vertices
    std::vector<MeshVertex> Vertices;

    for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
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

        Vertex.Tangent = glm::vec3(0.0f);
        if (mesh->HasTangentsAndBitangents()) Vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

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
    std::vector<Ref<Texture2D>> albedoTextures;
    std::vector<Ref<Texture2D>> normalTextures;
    std::vector<Ref<Texture2D>> metallicTextures;
    std::vector<Ref<Texture2D>> roughnessTextures;
    std::vector<Ref<Texture2D>> aoTextures;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        const auto albedos = LoadMaterialTextures(material, aiTextureType_DIFFUSE);
        albedoTextures.insert(albedoTextures.end(), albedos.begin(), albedos.end());

        const auto normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS);
        normalTextures.insert(normalTextures.end(), normalMaps.begin(), normalMaps.end());

        const auto metallic = LoadMaterialTextures(material, aiTextureType_METALNESS);
        metallicTextures.insert(metallicTextures.end(), metallic.begin(), metallic.end());

        const auto rougnhess = LoadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS);
        roughnessTextures.insert(roughnessTextures.end(), rougnhess.begin(), rougnhess.end());

        const auto ao = LoadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION);
        aoTextures.insert(aoTextures.end(), ao.begin(), ao.end());
    }

    Ref<Gauntlet::Material> Material = Material::Create();
    Material->m_AlbedoTextures       = albedoTextures;
    Material->m_NormalTextures       = normalTextures;
    Material->m_MetallicTextures     = metallicTextures;
    Material->m_RougnessTextures     = roughnessTextures;
    Material->m_AOTextures           = aoTextures;
    Material->Invalidate();

    return Submesh(mesh->mName.C_Str(), Vertices, Indices, Material);
}

std::vector<Ref<Texture2D>> Mesh::LoadMaterialTextures(aiMaterial* mat, aiTextureType type)
{
    std::vector<Ref<Texture2D>> Textures;
    const uint32_t TextureCount = mat->GetTextureCount(type);
    for (uint32_t i = 0; i < TextureCount; ++i)
    {
        aiString str;
        aiReturn res = mat->GetTexture(type, i, &str);
        if (res != aiReturn_SUCCESS) LOG_WARN("Failed to load texture %s", str.C_Str());

        const bool bIsLoaded = m_LoadedTextures.contains(str.C_Str());
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
        Ref<Texture2D> texture           = Ref<Texture2D>(Texture2D::Create(TexturePath, textureSpec));

        LOG_TRACE("Loaded texture: %s", TexturePath.data());

        Textures.emplace_back(texture);
        m_LoadedTextures[LocalTexturePath] = texture;
    }

    return Textures;
}

void Mesh::Destroy()
{
    for (auto& VertexBuffer : m_VertexBuffers)
        VertexBuffer->Destroy();

    for (auto& IndexBuffer : m_IndexBuffers)
        IndexBuffer->Destroy();

    for (auto& submesh : m_Submeshes)
        submesh.Material->Destroy();

    for (auto& LoadedTexture : m_LoadedTextures)
        LoadedTexture.second->Destroy();
}

}  // namespace Gauntlet