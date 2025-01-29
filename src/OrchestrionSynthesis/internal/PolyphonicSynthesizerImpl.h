#pragma once

#include "Orchestrion/IOrchestrion.h"
#include "Orchestrion/OrchestrionTypes.h"
#include <async/asyncable.h>
#include <modularity/ioc.h>

namespace dgk
{
class PolyphonicSynthesizerImpl : public muse::Injectable,
                                  public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;

public:
  PolyphonicSynthesizerImpl() = default;
  virtual ~PolyphonicSynthesizerImpl() = default;

protected:
  virtual void onVoicesReset() {}
  virtual void allNotesOff() = 0;
  void Initialize();
  int GetChannel(const TrackIndex &voice) const;

  std::vector<TrackIndex> m_voices;

private:
  void Setup();
};
} // namespace dgk
