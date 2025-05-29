#pragma once

#include "GestureControllerTypes.h"

#include <chrono>
#include <unordered_map>

namespace dgk
{
struct TouchpadContact
{
  int number = -1;
  float x = 0.f;
  float y = 0.f;
};

struct TouchpadScan
{
  const std::chrono::milliseconds scanTime;
  std::vector<TouchpadContact> contacts;
};
} // namespace dgk