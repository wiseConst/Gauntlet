#pragma once

#include "Core.h"
#include "InputCodes.h"

#include "Inherited.h"

namespace Eclipse
{
class Input : private Uncopyable, private Unmovable
{
  public:
    virtual ~Input() = default;

    static void Init();
    static FORCEINLINE void Destroy() { s_Instance->DestroyImpl(); }

    static FORCEINLINE bool IsKeyPressed(int KeyCode) { return s_Instance->IsKeyPressedImpl(KeyCode); }
    static FORCEINLINE bool IsMouseButtonPressed(int Button) { return s_Instance->IsMouseButtonPressedImpl(Button); }

    static FORCEINLINE std::pair<int, int> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
    static FORCEINLINE int GetMouseX() { return s_Instance->GetMouseXImpl(); }
    static FORCEINLINE int GetMouseY() { return s_Instance->GetMouseYImpl(); }

  protected:
    Input() = default;

  protected:
    static Input* s_Instance;

    virtual bool IsKeyPressedImpl(int KeyCode) const = 0;
    virtual bool IsMouseButtonPressedImpl(int Button) const = 0;

    virtual std::pair<int, int> GetMousePositionImpl() const = 0;
    virtual int GetMouseXImpl() const = 0;
    virtual int GetMouseYImpl() const = 0;

    virtual void DestroyImpl() = 0;
};
}  // namespace Eclipse