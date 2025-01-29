#include "PromisedSynthesizer.h"

namespace dgk
{
PromisedSynthesizer::PromisedSynthesizer(SynthPromise promise)
    : m_promise{std::move(promise)}
{
  m_promise.onReceive(
      this, [this](std::shared_ptr<IOrchestrionSynthesizer> synthesizer)
      { m_synthesizer = std::move(synthesizer); });
}

int PromisedSynthesizer::sampleRate() const
{
  return m_synthesizer ? m_synthesizer->sampleRate() : 0;
}

size_t PromisedSynthesizer::process(float *buffer, size_t samplesPerChannel)
{
  return m_synthesizer ? m_synthesizer->process(buffer, samplesPerChannel) : 0;
}

void PromisedSynthesizer::onNoteOns(size_t numNoteons, const TrackIndex* channels,
                                    const int *pitches, const float *velocities)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOns(numNoteons, channels, pitches, velocities);
}

void PromisedSynthesizer::onNoteOffs(size_t numNoteoffs, const TrackIndex* channels,
                                     const int *pitches)
{
  if (m_synthesizer)
    m_synthesizer->onNoteOffs(numNoteoffs, channels, pitches);
}

void PromisedSynthesizer::onPedal(bool on)
{
  if (m_synthesizer)
    m_synthesizer->onPedal(on);
}

} // namespace dgk