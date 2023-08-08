#include "GauntletPCH.h"
#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/TextureCube.h"

#include "Gauntlet/Renderer/Mesh.h"

namespace Gauntlet
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

}  // namespace Gauntlet