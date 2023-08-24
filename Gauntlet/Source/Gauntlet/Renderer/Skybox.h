#pragma once

namespace Gauntlet
{
class Mesh;
class TextureCube;

class Skybox final
{
  public:
    Skybox() = delete;
    Skybox(const std::vector<std::string>& faces);
    ~Skybox() = default;

    FORCEINLINE const auto& GetCubeMesh() const { return m_Cube; }
    FORCEINLINE const auto& GetCubeMapTexture() const { return m_CubeMapTexture; }

    void Destroy();

  private:
    Ref<TextureCube> m_CubeMapTexture;
    Ref<Mesh> m_Cube;

    void Create(const std::vector<std::string>& faces);
};

}  // namespace Gauntlet