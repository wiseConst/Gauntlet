#pragma once

#include "Eclipse/Core/Core.h"
#include "Eclipse/Renderer/CoreRendererStructs.h"
#include <vector>

#define GLM_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <tiny_obj_loader.h>

#include "Buffer.h"

#include "Eclipse/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Eclipse
{

class Mesh
{
  public:
    Mesh(const BufferInfo& InBufferInfo) : m_FilePath("NONE"), m_VertexBufferInfo(InBufferInfo)
    {
        Application::Get().GetThreadPool()->Enqueue([this] { LoadMesh(); });
        // std::async(std::launch::async, [this] { LoadMesh(); });
        // LoadMesh();
    }

    Mesh(const std::string_view& InFilePath) : m_FilePath(InFilePath)
    {
        Application::Get().GetThreadPool()->Enqueue([this] { LoadMesh(); });
        // std::async(std::launch::async, [this] { LoadMesh(); });
        // LoadMesh();
    }

    template <typename T> FORCEINLINE const Ref<T> GetVertexBuffer() const { return std::static_pointer_cast<T>(m_VertexBuffer); }

    template <typename T> FORCEINLINE Ref<T> GetVertexBuffer() { return std::static_pointer_cast<T>(m_VertexBuffer); }

    FORCEINLINE const auto& GetVertexBuffer() const { return m_VertexBuffer; }
    FORCEINLINE auto& GetVertexBuffer() { return m_VertexBuffer; }

    virtual ~Mesh() = default;
    virtual void Destroy() { m_VertexBuffer->Destroy(); }

  protected:
    BufferInfo m_VertexBufferInfo;
    Ref<VertexBuffer> m_VertexBuffer;
    const std::string_view m_FilePath;

  private:
    void LoadMesh()
    {
        if (strcmp(m_FilePath.data(), "NONE") != 0)
        {
            std::vector<Vertex> OutVertexData;
            LoadObject(m_FilePath, OutVertexData);

            BufferLayout VertexBufferLayout = {
                {EShaderDataType::Vec3, "InPosition"},  //
                {EShaderDataType::Vec3, "InNormal"},    //
                {EShaderDataType::Vec3, "InColor"}      //
            };                                          //

            BufferInfo VertexBufferInfo = {};
            VertexBufferInfo.Usage      = EBufferUsageFlags::VERTEX_BUFFER;
            VertexBufferInfo.Count      = OutVertexData.size();
            VertexBufferInfo.Size       = OutVertexData.size() * sizeof(OutVertexData[0]);
            VertexBufferInfo.Data       = OutVertexData.data();
            VertexBufferInfo.Layout     = VertexBufferLayout;

            m_VertexBuffer.reset(VertexBuffer::Create(VertexBufferInfo));
        }
        else
        {
            m_VertexBuffer.reset(VertexBuffer::Create(m_VertexBufferInfo));
        }
    }

    void LoadObject(const std::string_view& InFilePath, std::vector<Vertex>& OutVertexData)
    {
        // Attributes will contain the vertex arrays of the file
        tinyobj::attrib_t Attributes = {};

        // Shapes contains the info for each separate object in the file
        std::vector<tinyobj::shape_t> Shapes;

        // Materials contains the information about the material of each shape, but we won't use it.
        std::vector<tinyobj::material_t> Materials;

        // Error and warning output from the load function
        std::string Warning;
        std::string Error;

        tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Warning, &Error, InFilePath.data());
        if (!Warning.empty())
        {
            LOG_WARN("TinyObjWarn: %s", Warning);
        }

        // This happens if the file can't be found or is malformed
        if (!Error.empty())
        {
            LOG_ERROR("TinyObjWarn: %s", Error);

            return;
        }

        // Loop over shapes
        for (size_t s = 0; s < Shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < Shapes[s].mesh.num_face_vertices.size(); f++)
            {

                // hardcode loading to triangles
                int fv = 3;

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = Shapes[s].mesh.indices[index_offset + v];

                    // vertex position
                    tinyobj::real_t vx = Attributes.vertices[3 * idx.vertex_index + 0];
                    tinyobj::real_t vy = Attributes.vertices[3 * idx.vertex_index + 1];
                    tinyobj::real_t vz = Attributes.vertices[3 * idx.vertex_index + 2];
                    // vertex normal
                    tinyobj::real_t nx = Attributes.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = Attributes.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = Attributes.normals[3 * idx.normal_index + 2];

                    // copy it into our vertex
                    Vertex new_vert;
                    new_vert.Position.x = vx;
                    new_vert.Position.y = vy;
                    new_vert.Position.z = vz;

                    new_vert.Normal.x = nx;
                    new_vert.Normal.y = ny;
                    new_vert.Normal.z = nz;

                    // we are setting the vertex color as the vertex normal. This is just for display purposes
                    new_vert.Color = new_vert.Normal;

                    OutVertexData.push_back(new_vert);
                }
                index_offset += fv;
            }
        }
    }
};

}  // namespace Eclipse