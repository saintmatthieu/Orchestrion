#pragma once

#include "ExternalDevicesTypes.h"

namespace dgk
{
ExternalDeviceId::ExternalDeviceId(std::string id) : value{std::move(id)} {}

bool ExternalDeviceId::operator==(const ExternalDeviceId &other) const
{
  return value == other.value;
}

bool ExternalDeviceId::operator!=(const ExternalDeviceId &other) const
{
  return value != other.value;
}

bool operator==(const std::string &lhs, const ExternalDeviceId &rhs)
{
  return lhs == rhs.value;
}

bool operator==(const ExternalDeviceId &lhs, const std::string &rhs)
{
  return lhs.value == rhs;
}
} // namespace dgk