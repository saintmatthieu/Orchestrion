/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ControllerMenuManager.h"
#include <global/translation.h>
#include <log.h>

namespace dgk
{
namespace
{
const auto ignoreFailureKey =
    muse::Settings::Key{"midi", "ignore failure when setting MIDI controller"};
} // namespace

ControllerMenuManager::ControllerMenuManager()
    : DeviceMenuManager(DeviceType::MidiController)
{
}

std::string ControllerMenuManager::selectedDevice() const
{
  if (const std::optional<ExternalDeviceId> id =
          midiDeviceService()->selectedDevice())
    return id->value;
  return {};
}

muse::async::Notification ControllerMenuManager::availableDevicesChanged() const
{
  return midiDeviceService()->availableDevicesChanged();
}

std::vector<DeviceDesc> ControllerMenuManager::availableDevices() const
{
  const std::vector<ExternalDeviceId> ids =
      midiDeviceService()->availableDevices();
  std::vector<DeviceDesc> descriptions;
  descriptions.reserve(ids.size());
  std::transform(
      ids.begin(), ids.end(), std::back_inserter(descriptions),
      [this](const ExternalDeviceId &id)
      { return DeviceDesc{id.value, midiDeviceService()->deviceName(id)}; });
  return descriptions;
}

std::string ControllerMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseMidiDevice_" + std::to_string(deviceIndex);
}

bool ControllerMenuManager::selectDevice(const std::string &deviceId)
{
  const auto id = ExternalDeviceId{deviceId};
  if (midiDeviceService()->isAvailable(id))
  {
    midiDeviceService()->selectDevice(id);
    return true;
  }

  // I wrote this beautiful code, but for some reason, a few seconds after the
  // modal dialog is shown, the audio thread crashes.

  // Detail:
  // clang-format off
  // Call stack:
  // Orchestrion.exe!std::_Tree<std::_Tmap_traits<std::thread::id,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>,std::less<std::thread::id>,std::allocator<std::pair<std::thread::id const ,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>>>,0>>::_Find_lower_bound<std::thread::id>(const std::thread::id & _Keyval) Line 1599 (c:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\include\xtree:1599)
  // Orchestrion.exe!std::_Tree<std::_Tmap_traits<std::thread::id,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>,std::less<std::thread::id>,std::allocator<std::pair<std::thread::id const ,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>>>,0>>::_Find<std::thread::id>(const std::thread::id & _Keyval) Line 1354 (c:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\include\xtree:1354)
  // Orchestrion.exe!std::_Tree<std::_Tmap_traits<std::thread::id,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>,std::less<std::thread::id>,std::allocator<std::pair<std::thread::id const ,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>>>,0>>::find(const std::thread::id & _Keyval) Line 1363 (c:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\include\xtree:1363)
  // Orchestrion.exe!std::_Tree<std::_Tmap_traits<std::thread::id,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>,std::less<std::thread::id>,std::allocator<std::pair<std::thread::id const ,std::queue<std::function<void __cdecl(void)>,std::deque<std::function<void __cdecl(void)>,std::allocator<std::function<void __cdecl(void)>>>>>>,0>>::extract(const std::thread::id & _Keyval) Line 1731 (c:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\include\xtree:1731)
  // Orchestrion.exe!kors::async::QueuedInvoker::processEvents() Line 51 (c:\git\saintmatthieu\Orchestrion\MuseScore\src\framework\global\thirdparty\kors_async\async\internal\queuedinvoker.cpp:51)

  // The top of the call stack is doin4g
  // std::lock_guard<std::recursive_mutex> lock(m_mutex);
  //         auto n = m_queues.extract(std::this_thread::get_id());

  // and the exception is due to
  // template <class _Keyty>
  //     _Tree_find_result<_Nodeptr> _Find_lower_bound(const _Keyty& _Keyval) const {
  //         const auto _Scary = _Get_scary();
  //         _Tree_find_result<_Nodeptr> _Result{{_Scary->_Myhead->_Parent, _Tree_child::_Right}, _Scary->_Myhead};
  //         _Nodeptr _Trynode = _Result._Location._Parent;
  //         while (!_Trynode->_Isnil) {

  // causing
  // Exception has occurred: W32/0xC0000005
  // Unhandled exception thrown: read access violation.
  // _Trynode was 0xFFFFFFFFFFFFFFE6.
  // clang-format on

  // The crashing code looks quite easy and not faulty. My only guess is that
  // somehow the dialog corrupts some memory. Giving up for now, maybe I'll
  // coincidentally find out the reason later, at which point the call to
  // `maybePromptUser` can be uncommented.

  // return maybePromptUser(deviceId);
  return false;
}

bool ControllerMenuManager::maybePromptUser(const std::string &deviceId)
{
  if (const muse::Val ignoreFailureSetting =
          settings()->value(ignoreFailureKey);
      !ignoreFailureSetting.isNull() && ignoreFailureSetting.toBool())
    return false;

  const auto availableDevices = midiDeviceService()->availableDevices();
  const auto it = std::find_if(availableDevices.begin(), availableDevices.end(),
                               [&deviceId](const auto &device)
                               { return device.value == deviceId; });
  IF_ASSERT_FAILED(it != availableDevices.end())
  {
    LOGE() << "MIDI controller ID " + deviceId + " not found";
    return false;
  }

  using namespace muse;
  const IInteractive::Result result = interactive()->warning(
      muse::trc("midi", "Could not connect to MIDI controller ") +
          midiDeviceService()->deviceName(*it),
      muse::trc("midi",
                "You may check that it is not used by another application,\n"
                "select another controller or use the computer keyboard."),
      IInteractive::Buttons{IInteractive::Button::Discard,
                            IInteractive::Button::Retry},
      IInteractive::Button::NoButton,
      IInteractive::Options{IInteractive::Option::WithIcon |
                            IInteractive::Option::WithDontShowAgainCheckBox});

  switch (result.standardButton())
  {
  case IInteractive::Button::Discard:
    settings()->setSharedValue(ignoreFailureKey,
                               muse::Val{!result.showAgain()});
    return false;
  case IInteractive::Button::Retry:
    settings()->setSharedValue(ignoreFailureKey, muse::Val{false});
    return selectDevice(deviceId);
  default:
    assert(false);
    return false;
  }
}
} // namespace dgk