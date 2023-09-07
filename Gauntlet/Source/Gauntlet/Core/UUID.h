#pragma once

#include <xhash>

namespace Gauntlet
{

class UUID
{
  public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const UUID&) = default;

    operator uint64_t() const { return m_UUID; }

  private:
    uint64_t m_UUID;
};

}  // namespace Gauntlet

namespace std
{
template <> struct hash<Gauntlet::UUID>
{
    auto operator()(const Gauntlet::UUID& uuid) const -> size_t { return hash<uint64_t>()(uuid); }
};

}  // namespace std