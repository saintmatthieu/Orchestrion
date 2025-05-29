#include "GestureControllerConfigurator.h"
#include "GestureControllers/ITouchpadGestureController.h"

#include <log.h>

namespace dgk
{
void GestureControllerConfigurator::init()
{
  gestureControllerSelector()->selectedControllersChanged().onNotify(
      this,
      [this]
      {
        for (GestureControllerType type :
             gestureControllerSelector()->selectedControllers())
        {
          const auto controller =
              gestureControllerSelector()->getSelectedController(type);
          if (!controller)
            continue;
          controller->noteOn().onReceive(this, [this](int pitch, float velocity)
                                         { onNoteOn(pitch, velocity); });
          controller->noteOff().onReceive(this, [this](int pitch)
                                          { onNoteOff(pitch); });
        }
      });
}

void GestureControllerConfigurator::onNoteOn(int pitch, float velocity)
{
  const auto sequencer = orchestrion()->sequencer();
  IF_ASSERT_FAILED(sequencer) return;
  sequencer->OnInputEvent(NoteEventType::noteOn, pitch, velocity);
}

void GestureControllerConfigurator::onNoteOff(int pitch)
{
  const auto sequencer = orchestrion()->sequencer();
  IF_ASSERT_FAILED(sequencer) return;
  sequencer->OnInputEvent(NoteEventType::noteOff, pitch, 0.0f);
}

void GestureControllerConfigurator::setSelectedControllers(
    const GestureControllerTypeSet &types)
{
  gestureControllerSelector()->setSelectedControllers(types);
}
} // namespace dgk