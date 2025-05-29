#pragma once

#include <unordered_set>
#include <vector>


namespace dgk
{
enum class GestureControllerType
{
  MidiDevice,
  Touchpad,
  Swipe,
  ComputerKeyboard,
  _count
};

using GestureControllerTypeSet = std::unordered_set<GestureControllerType>;

struct Contact
{
  Contact(int id, float x, float y) : uid{id}, x{x}, y{y} {}
  const int uid;
  const float x;
  const float y;
};

using Contacts = std::vector<Contact>;
} // namespace dgk