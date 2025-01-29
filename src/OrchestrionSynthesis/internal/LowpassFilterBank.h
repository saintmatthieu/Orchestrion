#pragma once

#include "LowpassFilteredSynthesizer.h"
#include <array>
#include <functional>

namespace dgk
{
using SynthFactory =
    std::function<std::unique_ptr<IOrchestrionSynthesizer>(void)>;

class LowpassFilterBank : public IOrchestrionSynthesizer
{
public:
  LowpassFilterBank(const SynthFactory &);

private:
  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex* channels, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const TrackIndex* channels,
                  const int *pitches) override;
  void onPedal(bool on) override;

  static const auto numVelocitySteps = 9;
  std::array<std::shared_ptr<IOrchestrionSynthesizer>, numVelocitySteps>
      m_synthesizers;
  std::vector<float> m_mixBuffer;
  std::unordered_map<int, int> m_pitchesToSynthIndex;
  size_t m_maxSamples;
};
} // namespace dgk