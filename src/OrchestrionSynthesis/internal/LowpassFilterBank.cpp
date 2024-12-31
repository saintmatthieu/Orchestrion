#include "LowpassFilterBank.h"

namespace dgk
{
namespace
{
constexpr auto maxCutoff = 10000.0;
constexpr auto maxSamples = 4096;

} // namespace

LowpassFilterBank::LowpassFilterBank(SynthFactory synthFactory)
    : m_synthFactory{std::move(synthFactory)}, m_mixBuffer(maxSamples),
      m_maxSamples{maxSamples}
{
  for (auto i = 0; i < numVelocitySteps; ++i)
  {
    const auto velocity = static_cast<double>(i + 1) / numVelocitySteps;
    const auto cutoff = maxCutoff * velocity * velocity;
    m_synthesizers[i] =
        std::make_shared<LowpassFilteredSynthesizer>(m_synthFactory(), cutoff);
  }
}

int LowpassFilterBank::sampleRate() const
{
  return m_synthesizers[0]->sampleRate();
}

int LowpassFilterBank::numChannels() const
{
  return m_synthesizers[0]->numChannels();
}

size_t LowpassFilterBank::process(float *buffer, size_t samplesPerChannel)
{
  const auto numSamples = samplesPerChannel * m_synthesizers[0]->numChannels();
  IF_ASSERT_FAILED(numSamples <= m_maxSamples)
  {
    m_mixBuffer.resize(numSamples);
    m_maxSamples = numSamples;
  }
  std::fill(buffer, buffer + numSamples, 0.f);
  std::for_each(m_synthesizers.begin(), m_synthesizers.end(),
                [&](const auto &synth)
                {
                  synth->process(m_mixBuffer.data(), samplesPerChannel);
                  for (auto i = 0u; i < numSamples; ++i)
                    buffer[i] += m_mixBuffer[i];
                });
  return samplesPerChannel;
}

void LowpassFilterBank::onNoteOns(size_t numNoteons, const int *pitches,
                                  const float *velocities)
{
  for (auto i = 0u; i < numNoteons; ++i)
  {
    const auto velocity = velocities[i];
    const auto index = std::min<int>(velocity * (numVelocitySteps - 1) + .5,
                                     numVelocitySteps - 1);
    m_synthesizers[index]->onNoteOns(1, pitches + i, velocities + i);
    m_pitchesToSynthIndex[pitches[i]] = index;
  }
}

void LowpassFilterBank::onNoteOffs(size_t numNoteoffs, const int *pitches)
{
  for (auto i = 0u; i < numNoteoffs; ++i)
  {
    const auto it = m_pitchesToSynthIndex.find(pitches[i]);
    if (it == m_pitchesToSynthIndex.end())
      continue;
    m_synthesizers[it->second]->onNoteOffs(1, pitches + i);
    m_pitchesToSynthIndex.erase(it);
  }
}

void LowpassFilterBank::onPedal(bool on)
{
  for (auto &synth : m_synthesizers)
    synth->onPedal(on);
}

} // namespace dgk