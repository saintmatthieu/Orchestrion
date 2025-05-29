#pragma once

namespace dgk
{
namespace ExternalDevicesUtils
{
class ScopedTrue
{
public:
  ScopedTrue(bool &flag) : m_flag(flag) { m_flag = true; }
  ~ScopedTrue() { m_flag = false; }

private:
  bool &m_flag;
};
} // namespace ExternalDevicesUtils
} // namespace dgk