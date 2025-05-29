#pragma once

#include <string>

namespace dgk
{
struct DeviceDesc
{
  DeviceDesc(std::string id, std::string name)
      : id{std::move(id)}, name{std::move(name)}
  {
  }
  std::string id;
  std::string name;
};
} // namespace dgk