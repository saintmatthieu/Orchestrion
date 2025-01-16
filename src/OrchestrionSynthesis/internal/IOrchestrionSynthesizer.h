#pragma once

#include "Orchestrion/OrchestrionTypes.h"

namespace dgk
{
class IOrchestrionSynthesizer
{
public:
  virtual ~IOrchestrionSynthesizer() = default;

  static constexpr auto numChannels = 2;

  virtual int sampleRate() const = 0;
  virtual size_t process(float *buffer, size_t samplesPerChannel) = 0;
  virtual void onNoteOns(size_t numNoteons, const int *pitches,
                         const float *velocities) = 0;
  virtual void onNoteOffs(size_t numNoteoffs, const int *pitches) = 0;
  virtual void onPedal(bool on) = 0;
};
} // namespace dgk
