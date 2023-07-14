#pragma once

#include "Eclipse/Core/Core.h"

namespace Eclipse
{

class AABB : private Uncopyable, private Unmovable
{
  public:
    AABB()  = default;
    ~AABB() = default;

  private:
};
}  // namespace Eclipse