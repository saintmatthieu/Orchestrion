#pragma once

#include "IOrchestrionSynthesizer.h"
#include <DSPFilters/Bessel.h>
#include <DSPFilters/SmoothedFilter.h>

namespace dgk
{
class LowpassFilteredSynthesizer : public IOrchestrionSynthesizer
{
public:
  LowpassFilteredSynthesizer(std::unique_ptr<IOrchestrionSynthesizer>,
                             double cutoff);
  ~LowpassFilteredSynthesizer();

private:
  int sampleRate() const override;
  int numChannels() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const int *pitches) override;
  void onPedal(bool on) override;

  void initBuffers(size_t samplesPerChannel);
  void deleteBuffers();

  const std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;

  static const auto order = 2;
  static const auto audioChannelCount = 2;
  Dsp::SimpleFilter<Dsp::Bessel::LowPass<order>, audioChannelCount>
      m_lowPassFilter;
  float **m_audioBuffer = nullptr;
  size_t m_maxSamplesPerChannel;
};
} // namespace dgk