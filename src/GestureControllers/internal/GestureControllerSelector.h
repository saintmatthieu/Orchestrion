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
#pragma once

#include "ComputerKeyboard/ComputerKeyboardGestureController.h"
#include "ExternalDevices/IMidiDeviceService.h"
#include "IGestureController.h"
#include "IGestureControllerConfiguration.h"
#include "IGestureControllerSelector.h"
#include "MidiDevice/MidiDeviceGestureController.h"
#include "Touchpad/ITouchpad.h"
#include "Touchpad/TouchpadGestureController.h"

#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"

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
  void addSelectedController(GestureControllerType) override;
  muse::async::Notification selectedControllersChanged() const override;
  GestureControllerTypeSet selectedControllers() const override;

  const IGestureController *
      getSelectedController(GestureControllerType) const override;

  muse::async::Notification touchpadControllerChanged() const override;
  const ITouchpadGestureController *getTouchpadController() const override;

  muse::async::Notification activityDetectedOnMidiControllerWhileDeselected() const override;

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
  muse::async::Notification m_activityDetectedOnMidiControllerWhileDeselected;
};
} // namespace dgk