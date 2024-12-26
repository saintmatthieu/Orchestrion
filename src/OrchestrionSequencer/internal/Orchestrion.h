#pragma once

#include "IOrchestrion.h"
#include "OrchestrionSequencer.h"
#include "playback/iplaybackcontroller.h"
#include <async/asyncable.h>
#include <context/iglobalcontext.h>

namespace dgk
{
class Orchestrion : public IOrchestrion,
                    public muse::async::Asyncable,
                    public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController = {this};
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};

public:
  void init();

private:
  IOrchestrionSequencer *sequencer() override;
  muse::async::Notification sequencerChanged() const override;
  void setSequencer(std::unique_ptr<IOrchestrionSequencer> sequencer);

private:
  std::unique_ptr<IOrchestrionSequencer> m_sequencer;
  muse::async::Notification m_sequencerChanged;
};
} // namespace dgk