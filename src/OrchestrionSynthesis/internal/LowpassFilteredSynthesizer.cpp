#include "LowpassFilteredSynthesizer.h"

namespace dgk
{
namespace
{
// When re-parametrizing, how many samples used in the transition from old to
// new coefs.
constexpr auto interpolationSamples = 16;
constexpr auto leastTimeBetweenReparametrizations = 0.05;
} // namespace

LowpassFilteredSynthesizer::LowpassFilteredSynthesizer(
    std::unique_ptr<IOrchestrionSynthesizer> synthesizer)
    : m_synthesizer{std::move(synthesizer)},
      m_lowPassFilter{interpolationSamples}
{
  m_audioBuffer = new float *[numChannels()];
  for (auto i = 0; i < numChannels(); ++i)
    m_audioBuffer[i] = new float[maxSamplesPerChannel];

  setCutoff(m_synthesizer->sampleRate() / 2);
}

LowpassFilteredSynthesizer::~LowpassFilteredSynthesizer()
{
  for (auto i = 0; i < numChannels(); ++i)
    delete[] m_audioBuffer[i];
  delete[] m_audioBuffer;
}

void LowpassFilteredSynthesizer::setCutoff(double cutoff)
{
  Dsp::Params params;
  params[0] = m_synthesizer->sampleRate();
  params[1] = lowpassOrder;
  params[2] = cutoff;
  m_lowPassFilter.setParams(params);
}

int LowpassFilteredSynthesizer::sampleRate() const
{
  return m_synthesizer->sampleRate();
}

int LowpassFilteredSynthesizer::numChannels() const
{
  return m_synthesizer->numChannels();
}

size_t LowpassFilteredSynthesizer::process(float *buffer,
                                           size_t samplesPerChannel)
{
  const auto produced = m_synthesizer->process(buffer, samplesPerChannel);
  const auto C = numChannels();

  // Deinterleave
  for (auto i = 0; i < C; ++i)
    for (muse::audio::samples_t j = 0; j < samplesPerChannel; ++j)
      m_audioBuffer[i][j] = buffer[j * C + i];

  m_lowPassFilter.process((int)samplesPerChannel, m_audioBuffer);

  // Re-Interleave
  for (auto i = 0; i < C; ++i)
    for (muse::audio::samples_t j = 0; j < samplesPerChannel; ++j)
      buffer[j * C + i] = m_audioBuffer[i][j];

  m_samplesSinceReparametrization += (int)samplesPerChannel;

  return produced;
}

void LowpassFilteredSynthesizer::onNoteOns(size_t numNoteons,
                                           const int *pitches,
                                           const float *velocities)
{
  // Assuming (as elswewhere in this file) that calls are made from the same
  // thread.
  if (m_samplesSinceReparametrization >=
      leastTimeBetweenReparametrizations * sampleRate())
  {
    const auto max = *std::max_element(velocities, velocities + numNoteons);
    setCutoff(max * max * 10000);
    m_samplesSinceReparametrization = 0;
  }

  m_synthesizer->onNoteOns(numNoteons, pitches, velocities);
}

void LowpassFilteredSynthesizer::onNoteOffs(size_t numNoteoffs,
                                            const int *pitches)
{
  m_synthesizer->onNoteOffs(numNoteoffs, pitches);
}

void LowpassFilteredSynthesizer::onPedal(bool on)
{
  m_synthesizer->onPedal(on);
}

} // namespace dgk