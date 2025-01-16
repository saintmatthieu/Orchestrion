#include "OrchestrionSynthResolver.h"
#include "AntiMetronomeSynthesizer.h"
#include "OrchestrionSynthesizerWrapper.h"
#include "OrchestrionVstSynthesizer.h"
#include "PromisedSynthesizer.h"

namespace dgk
{
void OrchestrionSynthResolver::resolveToVst(
    const muse::audio::AudioResourceId &id)
{
  m_vstId = id;
  m_synthType = SynthType::Vst;
}

void OrchestrionSynthResolver::resolveToFluid()
{
  m_vstId.reset();
  m_synthType = SynthType::Fluid;
}

void OrchestrionSynthResolver::resolveToNone()
{
  m_vstId.reset();
  m_synthType = SynthType::None;
}

muse::audio::synth::ISynthesizerPtr OrchestrionSynthResolver::resolveSynth(
    const muse::audio::TrackId trackId,
    const muse::audio::AudioInputParams &) const
{
  return std::make_shared<OrchestrionSynthesizerWrapper>(
      [this, trackId, vstId = m_vstId](
          int sampleRate) -> std::unique_ptr<IOrchestrionSynthesizer>
      {
        if (vstId.has_value())
        {
          muse::async::Channel<std::shared_ptr<IOrchestrionSynthesizer>>
              synthLoaded;
          const auto pluginPtr = std::make_shared<muse::vst::VstPlugin>(*vstId);
          pluginPtr->loadingCompleted().onNotify(
              this,
              [sampleRate, trackId, synthLoaded, pluginPtr]() mutable
              {
                synthLoaded.send(std::make_shared<AntiMetronomeSynthesizer>(
                    sampleRate, trackId,
                    [pluginPtr](int sampleRate)
                    {
                      return std::make_unique<OrchestrionVstSynthesizer>(
                          pluginPtr, sampleRate);
                    }));
              });
          pluginPtr->load();
          return std::make_unique<PromisedSynthesizer>(synthLoaded);
        }
        else if (m_synthType == SynthType::Fluid)
          return std::make_unique<AntiMetronomeSynthesizer>(
              sampleRate, trackId,
              [](int sampleRate)
              {
                return std::make_unique<LowpassFilterBank>(
                    [sampleRate]
                    { return std::make_unique<FluidSynthesizer>(sampleRate); });
              });
        else
          return nullptr;
      });
}

bool OrchestrionSynthResolver::hasCompatibleResources(
    const muse::audio::PlaybackSetupData &) const
{
  return true;
}

muse::audio::AudioResourceMetaList
OrchestrionSynthResolver::resolveResources() const
{
  return {};
}

muse::audio::SoundPresetList OrchestrionSynthResolver::resolveSoundPresets(
    const muse::audio::AudioResourceMeta &resourceMeta) const
{
  return {};
}

void OrchestrionSynthResolver::refresh() {}

void OrchestrionSynthResolver::clearSources() {}
} // namespace dgk