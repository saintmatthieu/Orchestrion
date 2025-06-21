#include "PolyphonicSynthesizerImpl.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"

namespace dgk
{
void PolyphonicSynthesizerImpl::Initialize()
{
  Setup();
  orchestrion()->sequencerChanged().onNotify(this, [this] { Setup(); });
}

void PolyphonicSynthesizerImpl::Setup()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  m_voices = sequencer->GetAllVoices();
  onVoicesReset();
  sequencer->AboutToJumpPosition().onNotify(this, [this] { allNotesOff(); });
}

int PolyphonicSynthesizerImpl::GetChannel(const TrackIndex &voice) const
{
  return std::distance(m_voices.begin(),
                       std::find(m_voices.begin(), m_voices.end(), voice));
}

} // namespace dgk