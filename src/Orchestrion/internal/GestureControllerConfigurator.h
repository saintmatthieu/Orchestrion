#pragma once

#include "GestureControllers/IGestureControllerSelector.h"
#include "IGestureControllerConfigurator.h"
#include "IOrchestrion.h"


#include "async/asyncable.h"
#include "modularity/ioc.h"

namespace dgk
{
class GestureControllerConfigurator : public IGestureControllerConfigurator,
                                      public muse::Injectable,
                                      public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IGestureControllerSelector> gestureControllerSelector;

public:
  void init();

private:
  void setSelectedControllers(const GestureControllerTypeSet &) override;

  void onNoteOn(int, float);
  void onNoteOff(int);
};
} // namespace dgk