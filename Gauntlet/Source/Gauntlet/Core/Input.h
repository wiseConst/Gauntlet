#pragma once

#include "Core.h"
#include "InputCodes.h"

namespace Gauntlet
{

class Input final : private Uncopyable, private Unmovable
{
  public:
    virtual ~Input() = default;

    static void Init();
    static void Destroy();

    static bool IsKeyPressed(int32_t KeyCode);
    static bool IsMouseButtonPressed(int32_t Button);

    static bool IsKeyReleased(int32_t KeyCode);
    static bool IsMouseButtonReleased(int32_t Button);

    static std::pair<int32_t, int32_t> GetMousePosition();
    static int32_t GetMouseX();
    static int32_t GetMouseY();

  private:
    static Input* s_Instance;

    Input() = default;
};
}  // namespace Gauntlet