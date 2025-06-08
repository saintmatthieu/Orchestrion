#pragma once

#include "ComputerKeyboard/ComputerKeyboardGestureController.h"
#include "ExternalDevices/IMidiDeviceService.h"
#include "IGestureController.h"
#include "IGestureControllerConfiguration.h"
#include "IGestureControllerSelector.h"
#include "MidiDevice/MidiDeviceGestureController.h"
#include "Touchpad/ITouchpad.h"
#include "Touchpad/TouchpadGestureController.h"

#include "async/asyncable.h"
#include "modularity/ioc.h"

namespace dgk
{
class GestureControllerSelector : public IGestureControllerSelector,
                                  public muse::async::Asyncable,
                                  public muse::Injectable
{
public:
  void init();

public:
  GestureControllerTypeSet functionalControllers() const override;

  void setSelectedControllers(GestureControllerTypeSet) override;
  muse::async::Notification selectedControllersChanged() const override;
  GestureControllerTypeSet selectedControllers() const override;

  const IGestureController *
      getSelectedController(GestureControllerType) const override;

  muse::async::Notification touchpadControllerChanged() const override;
  const ITouchpadGestureController *getTouchpadController() const override;

private:
  void doStartupSelection();
  void doFallbackSelection();
  std::vector<const IGestureController *> allControllers() const;
  bool doSetSelectedControllers(GestureControllerTypeSet types);

  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<IGestureControllerConfiguration> configuration;

  std::unique_ptr<ITouchpad> m_touchpad;
  std::unique_ptr<TouchpadGestureController> m_touchpadController;
  std::unique_ptr<ComputerKeyboardGestureController>
      m_computerKeyboardController;
  std::unique_ptr<MidiDeviceGestureController> m_midiDeviceController;
  muse::async::Notification m_touchpadControllerChanged;
  muse::async::Notification m_selectedControllersChanged;
};
} // namespace dgk