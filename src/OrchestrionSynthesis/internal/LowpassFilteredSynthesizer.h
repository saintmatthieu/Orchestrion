#pragma once

#include "IOrchestrionSynthesizer.h"
#include <DSPFilters/Bessel.h>
#include <DSPFilters/SmoothedFilter.h>

namespace dgk
{
class LowpassFilteredSynthesizer : public IOrchestrionSynthesizer
{
public:
  LowpassFilteredSynthesizer(std::unique_ptr<IOrchestrionSynthesizer>);
  ~LowpassFilteredSynthesizer();

private:
  int sampleRate() const override;
  int numChannels() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const int *pitches) override;
  void onPedal(bool on) override;

  void setCutoff(double cutoff);

  const std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;

  static const auto lowpassOrder = 2;
  static const auto audioChannelCount = 2;
  Dsp::SmoothedFilterDesign<Dsp::Bessel::Design::LowPass<lowpassOrder>,
                            audioChannelCount>
      m_lowPassFilter;
  float **m_audioBuffer = nullptr;

  int m_samplesSinceReparametrization = 0;
};
} // namespace dgk