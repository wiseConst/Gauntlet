#pragma once

// Should be always inherited as private.

namespace Gauntlet
{

class Uncopyable
{
  protected:
    Uncopyable()  = default;
    ~Uncopyable() = default;

  private:
    Uncopyable(const Uncopyable&) = delete;
    Uncopyable& operator=(const Uncopyable&) = delete;
};

class Unmovable
{
  protected:
    Unmovable()  = default;
    ~Unmovable() = default;

  private:
    Unmovable(Unmovable&&) = delete;
    Unmovable& operator=(Unmovable&&) = delete;
};

}  // namespace Gauntlet