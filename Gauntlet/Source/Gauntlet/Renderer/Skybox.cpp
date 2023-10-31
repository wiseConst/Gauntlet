#include "GauntletPCH.h"
#include "Gauntlet/Renderer/Skybox.h"
#include "Gauntlet/Renderer/TextureCube.h"

#include "Gauntlet/Renderer/Mesh.h"

namespace Gauntlet
{
Skybox::Skybox(const std::vector<std::string>& faces)
{
    Create(faces);
}

void Skybox::Create(const std::vector<std::string>& faces)
{
    GNT_ASSERT(false);
    // m_Cube = Mesh::CreateCube();

    m_CubeMapTexture = TextureCube::Create(faces);
}

void Skybox::Destroy()
{
    m_CubeMapTexture->Destroy();
    m_Cube->Destroy();
}

}  // namespace Gauntlet