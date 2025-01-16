#pragma once

#include <ISynthesizerConnector.h>
#include <async/asyncable.h>
#include <audio/isynthresolver.h>
#include <modularity/ioc.h>
#include <optional>

namespace dgk
{
class OrchestrionSynthResolver
    : public muse::audio::synth::ISynthResolver::IResolver,
      public muse::Injectable,
      public muse::async::Asyncable
{
public:
  void resolveToVst(const muse::audio::AudioResourceId &);
  void resolveToFluid();
  void resolveToNone();

private:
  muse::Inject<ISynthesizerConnector> synthesizerConnector;

private:
  muse::audio::synth::ISynthesizerPtr
  resolveSynth(const muse::audio::TrackId,
               const muse::audio::AudioInputParams &) const override;

  bool
  hasCompatibleResources(const muse::audio::PlaybackSetupData &) const override;

  muse::audio::AudioResourceMetaList resolveResources() const override;

  muse::audio::SoundPresetList
  resolveSoundPresets(const muse::audio::AudioResourceMeta &) const override;

  void refresh() override;

  void clearSources() override;

  enum class SynthType
  {
    None,
    Vst,
    Fluid
  };

  SynthType m_synthType = SynthType::None;
  std::optional<muse::audio::AudioResourceId> m_vstId;
};
} // namespace dgk