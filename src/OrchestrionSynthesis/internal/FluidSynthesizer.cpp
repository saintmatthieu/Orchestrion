#include "FluidSynthesizer.h"
#include <fluidsynth.h>

namespace muse::audio::synth
{
// This is inline-defined in sfcachedloader.h, but including sfcachedloader.h
// yields a linker error about duplicate symbols.
// Since the linker finds the definition, we don't include the header and just
// add the missing declaration here.
fluid_sfont_t *loadSoundFont(fluid_sfloader_t *loader, const char *filename);
} // namespace muse::audio::synth

namespace dgk
{
namespace
{
constexpr double globalVolumeGain = 4.8;
constexpr int defaultMidiVolume = 100;
constexpr int minNoteLenMs = 10;
constexpr unsigned int audioChannelPairs = 1;
constexpr unsigned int audioChannelCount = audioChannelPairs * 2;
constexpr int midiChannel = 0;
} // namespace

FluidSynthesizer::FluidSynthesizer(int sampleRate) : m_sampleRate{sampleRate}
{
  m_fluidSettings = new_fluid_settings();
  fluid_settings_setnum(m_fluidSettings, "synth.gain", globalVolumeGain);
  fluid_settings_setint(m_fluidSettings, "synth.audio-channels",
                        audioChannelPairs);
  fluid_settings_setint(m_fluidSettings, "synth.lock-memory", 0);
  fluid_settings_setint(m_fluidSettings, "synth.threadsafe-api", 0);
  fluid_settings_setint(m_fluidSettings, "synth.midi-channels", 16);
  fluid_settings_setint(m_fluidSettings, "synth.dynamic-sample-loading", 1);
  fluid_settings_setint(m_fluidSettings, "synth.polyphony", 512);
  fluid_settings_setint(m_fluidSettings, "synth.min-note-length", minNoteLenMs);
  fluid_settings_setint(m_fluidSettings, "synth.chorus.active", 0);
  fluid_settings_setint(m_fluidSettings, "synth.reverb.active", 0);
  fluid_settings_setstr(m_fluidSettings, "audio.sample-format", "float");
  fluid_settings_setnum(m_fluidSettings, "synth.sample-rate",
                        static_cast<double>(m_sampleRate));

  m_fluidSynth = new_fluid_synth(m_fluidSettings);
  auto sfloader = new_fluid_sfloader(muse::audio::synth::loadSoundFont,
                                     delete_fluid_sfloader);
  fluid_sfloader_set_data(sfloader, m_fluidSettings);
  fluid_synth_add_sfloader(m_fluidSynth, sfloader);

  const auto soundFonts = soundFontRepository()->soundFonts();
  std::for_each(
      soundFonts.begin(), soundFonts.end(),
      [this](const std::pair<muse::audio::synth::SoundFontPath,
                             muse::audio::synth::SoundFontMeta> &entry)
      { fluid_synth_sfload(m_fluidSynth, entry.first.c_str(), 0); });

  fluid_synth_activate_key_tuning(m_fluidSynth, 0, 0, "standard", NULL, true);
  fluid_synth_set_interp_method(m_fluidSynth, midiChannel,
                                FLUID_INTERP_DEFAULT);
  fluid_synth_pitch_wheel_sens(m_fluidSynth, midiChannel, 24);

  // The following will become relevant when we allow the user to change the
  // sound. At the moment we just use piano.
  fluid_synth_bank_select(m_fluidSynth, midiChannel, 0);
  fluid_synth_program_change(m_fluidSynth, midiChannel, 0);

  fluid_synth_cc(m_fluidSynth, midiChannel, 7, defaultMidiVolume);
  fluid_synth_cc(m_fluidSynth, midiChannel, 74, 0);
  fluid_synth_set_portamento_mode(m_fluidSynth, midiChannel,
                                  FLUID_CHANNEL_PORTAMENTO_MODE_EACH_NOTE);
  fluid_synth_set_legato_mode(m_fluidSynth, midiChannel,
                              FLUID_CHANNEL_LEGATO_MODE_RETRIGGER);
  fluid_synth_activate_tuning(m_fluidSynth, midiChannel, 0, 0, 0);
}

FluidSynthesizer::~FluidSynthesizer()
{
  delete_fluid_synth(m_fluidSynth);
  delete_fluid_settings(m_fluidSettings);
}

int FluidSynthesizer::sampleRate() const { return m_sampleRate; }

int FluidSynthesizer::numChannels() const { return audioChannelCount; }

size_t FluidSynthesizer::process(float *buffer, size_t samplesPerChannel)
{
  fluid_synth_write_float(m_fluidSynth, (int)samplesPerChannel, buffer, 0,
                          audioChannelCount, buffer, 1, audioChannelCount);
  return samplesPerChannel;
}

void FluidSynthesizer::onNoteOns(size_t numNoteons, const int *pitches,
                                 const float *velocities)
{
  for (auto i = 0u; i < numNoteons; ++i)
    fluid_synth_noteon(m_fluidSynth, midiChannel, pitches[i],
                       velocities[i] * 127 + .5f);
}

void FluidSynthesizer::onNoteOffs(size_t numNoteoffs, const int *pitches)
{
  for (auto i = 0u; i < numNoteoffs; ++i)
    fluid_synth_noteoff(m_fluidSynth, midiChannel, pitches[i]);
}

void FluidSynthesizer::onPedal(bool on)
{
  fluid_synth_cc(m_fluidSynth, midiChannel, 0x40, on ? 127 : 0);
}
} // namespace dgk