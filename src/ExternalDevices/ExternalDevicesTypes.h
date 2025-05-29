#pragma once

#include <string>

namespace dgk
{
struct ExternalDeviceId
{
  explicit ExternalDeviceId(std::string id);

  ExternalDeviceId &operator=(const ExternalDeviceId &other) = default;

  bool operator==(const ExternalDeviceId &other) const;
  bool operator!=(const ExternalDeviceId &other) const;

  std::string value;
};

bool operator==(const std::string &lhs, const ExternalDeviceId &rhs);
bool operator==(const ExternalDeviceId &lhs, const std::string &rhs);

} // namespace dgk