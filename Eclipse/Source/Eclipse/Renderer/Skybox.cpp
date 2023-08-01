#include "EclipsePCH.h"
#include "Eclipse/Renderer/Skybox.h"
#include "Eclipse/Renderer/TextureCube.h"

#include "Eclipse/Renderer/Mesh.h"

namespace Eclipse
{
Skybox::Skybox(const std::vector<std::string>& InFaces)
{
    Create(InFaces);
}

void Skybox::Create(const std::vector<std::string>& InFaces)
{
    m_Cube = Mesh::CreateCube();

    m_CubeMapTexture = TextureCube::Create(InFaces);
}

void Skybox::Destroy()
{
    m_CubeMapTexture->Destroy();
    m_Cube->Destroy();
}

}  // namespace Eclipse