#pragma once

#include "Gauntlet/Core/Core.h"

namespace Gauntlet
{

class FileDialogs final
{
  public:
    // Returns empty string if canceled.
    static std::string OpenFile(const std::string_view& filter);
    static std::string SaveFile(const std::string_view& filter);

  private:
};

}  // namespace Gauntlet