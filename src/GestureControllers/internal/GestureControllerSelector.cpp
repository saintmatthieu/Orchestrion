#include "GestureControllerSelector.h"
#include "IGestureController.h"
#include "Touchpad/SwipeGestureController.h"
#include "Touchpad/Touchpad.h"
#include "Touchpad/TouchpadGestureController.h"

#include <cassert>

namespace dgk
{
void GestureControllerSelector::init()
{
  midiDeviceService()->selectedDeviceChanged().onNotify(
      this, [this] { doFallbackSelection(); });

  midiDeviceService()->startupSelectionFinished().onNotify(
      this, [this] { doStartupSelection(); });
}

void GestureControllerSelector::doStartupSelection()
{
  const auto configControllers = configuration()->readSelectedControllers();
  if (!configControllers)
  {
    const auto midiDevice = midiDeviceService()->selectedDevice();
    if (!midiDevice || !midiDeviceService()->isAvailable(*midiDevice) ||
        midiDeviceService()->isNoDevice(*midiDevice))
      doSetSelectedControllers({GestureControllerType::ComputerKeyboard});
    else
      doSetSelectedControllers({GestureControllerType::MidiDevice});
    return;
  }
  else
    doSetSelectedControllers(*configControllers);
}

void GestureControllerSelector::doFallbackSelection()
{
  if (configuration()->readSelectedControllers())
    return;
  const auto midiDevice = midiDeviceService()->selectedDevice();
  if (!midiDevice)
    doSetSelectedControllers({GestureControllerType::ComputerKeyboard});
  else if (!midiDeviceService()->isAvailable(*midiDevice) ||
           midiDeviceService()->isNoDevice(*midiDevice))
    doSetSelectedControllers({GestureControllerType::ComputerKeyboard,
                              GestureControllerType::MidiDevice});
  else
    doSetSelectedControllers({GestureControllerType::MidiDevice});
}

GestureControllerTypeSet
GestureControllerSelector::functionalControllers() const
{
  GestureControllerTypeSet types;
  if (MidiDeviceGestureController::isFunctional())
    types.insert(GestureControllerType::MidiDevice);
  if (TouchpadGestureController::isFunctional())
    types.insert(GestureControllerType::Touchpad);
  if (ComputerKeyboardGestureController::isFunctional())
    types.insert(GestureControllerType::ComputerKeyboard);
  if (SwipeGestureController::isFunctional())
    types.insert(GestureControllerType::Swipe);
  return types;
}

void GestureControllerSelector::setSelectedControllers(
    GestureControllerTypeSet types)
{
  if (doSetSelectedControllers(types))
    configuration()->writeSelectedControllers(selectedControllers());
}

bool GestureControllerSelector::doSetSelectedControllers(
    GestureControllerTypeSet types)
{
  bool changed = false;

  {
    const auto needsTouchpad =
        types.find(GestureControllerType::Touchpad) != types.end();

    if (m_touchpadController && !needsTouchpad)
    {
      changed = true;
      m_touchpad.reset();
      m_touchpadController.reset();
      m_touchpadControllerChanged.notify();
    }
    else if (!m_touchpadController && needsTouchpad &&
             TouchpadGestureController::isFunctional())
    {
      changed = true;
      m_touchpad = std::make_unique<Touchpad>();
      m_touchpadController =
          std::make_unique<TouchpadGestureController>(*m_touchpad);
      m_touchpadControllerChanged.notify();
    }
  }

  {
    const auto needsComputerKeyboard =
        types.find(GestureControllerType::ComputerKeyboard) != types.end();

    if (m_computerKeyboardController && !needsComputerKeyboard)
    {
      changed = true;
      m_computerKeyboardController.reset();
    }
    else if (!m_computerKeyboardController && needsComputerKeyboard &&
             ComputerKeyboardGestureController::isFunctional())
    {
      changed = true;
      m_computerKeyboardController =
          std::make_unique<ComputerKeyboardGestureController>();
    }
  }

  {
    const auto needsMidiDevice =
        types.find(GestureControllerType::MidiDevice) != types.end();

    if (m_midiDeviceController && !needsMidiDevice)
    {
      changed = true;
      m_midiDeviceController.reset();
    }
    else if (!m_midiDeviceController && needsMidiDevice &&
             MidiDeviceGestureController::isFunctional())
    {
      m_midiDeviceController = std::make_unique<MidiDeviceGestureController>();
      changed = true;
    }
  }

  if (changed)
    m_selectedControllersChanged.notify();

  return changed;
}

muse::async::Notification
GestureControllerSelector::selectedControllersChanged() const
{
  return m_selectedControllersChanged;
}

GestureControllerTypeSet GestureControllerSelector::selectedControllers() const
{
  GestureControllerTypeSet types;
  if (m_touchpadController)
    types.insert(GestureControllerType::Touchpad);
  if (m_computerKeyboardController)
    types.insert(GestureControllerType::ComputerKeyboard);
  if (m_midiDeviceController)
    types.insert(GestureControllerType::MidiDevice);
  return types;
}

const IGestureController *GestureControllerSelector::getSelectedController(
    GestureControllerType type) const
{
  switch (type)
  {
  case GestureControllerType::MidiDevice:
    return m_midiDeviceController.get();
  case GestureControllerType::Touchpad:
    return m_touchpadController.get();
  case GestureControllerType::Swipe:
    return nullptr;
  case GestureControllerType::ComputerKeyboard:
    return m_computerKeyboardController.get();
  default:
    assert(false);
    return nullptr;
  }
}

muse::async::Notification
GestureControllerSelector::touchpadControllerChanged() const
{
  return m_touchpadControllerChanged;
}

const ITouchpadGestureController *
GestureControllerSelector::getTouchpadController() const
{
  return m_touchpadController.get();
}
} // namespace dgk