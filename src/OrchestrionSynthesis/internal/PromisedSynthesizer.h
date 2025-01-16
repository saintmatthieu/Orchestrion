#pragma once

#include "IOrchestrionSynthesizer.h"
#include <async/asyncable.h>
#include <async/channel.h>

namespace dgk
{
class PromisedSynthesizer : public IOrchestrionSynthesizer,
                            public muse::async::Asyncable
{
public:
  using SynthPromise =
      muse::async::Channel<std::shared_ptr<IOrchestrionSynthesizer>>;

  PromisedSynthesizer(SynthPromise promise);

  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const int *pitches) override;
  void onPedal(bool on) override;

private:
  SynthPromise m_promise;
  std::shared_ptr<IOrchestrionSynthesizer> m_synthesizer;
};
} // namespace dgk