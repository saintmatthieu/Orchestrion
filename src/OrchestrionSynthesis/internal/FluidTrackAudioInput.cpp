/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "FluidTrackAudioInput.h"
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
constexpr double FLUID_GLOBAL_VOLUME_GAIN = 4.8;
constexpr int DEFAULT_MIDI_VOLUME = 100;
constexpr muse::audio::msecs_t MIN_NOTE_LENGTH = 10;
constexpr unsigned int FLUID_AUDIO_CHANNELS_PAIR = 1;
constexpr unsigned int FLUID_AUDIO_CHANNELS_COUNT =
    FLUID_AUDIO_CHANNELS_PAIR * 2;
} // namespace

FluidTrackAudioInput::~FluidTrackAudioInput() { destroySynth(); }

void FluidTrackAudioInput::processEvent(const EventVariant &event)
{
  if (std::holds_alternative<dgk::NoteEvents>(event))
    for (const auto &evt : std::get<dgk::NoteEvents>(event))
    {
      switch (evt.type)
      {
      case NoteEvent::Type::noteOn:
        fluid_synth_noteon(m_fluidSynth, evt.channel, evt.pitch,
                           evt.velocity * 127 + .5f);
        break;
      case NoteEvent::Type::noteOff:
        fluid_synth_noteoff(m_fluidSynth, evt.channel, evt.pitch);
        break;
      default:
        assert(false);
      }
    }
  else if (std::holds_alternative<dgk::PedalEvent>(event))
  {
    const auto &pedalEvent = std::get<dgk::PedalEvent>(event);
    fluid_synth_cc(m_fluidSynth, pedalEvent.channel, 0x40,
                   pedalEvent.on ? 127 : 0);
  }
}

bool FluidTrackAudioInput::_isActive() const { return m_fluidSynth != nullptr; }

void FluidTrackAudioInput::_setIsActive(bool) { assert(false); }

void FluidTrackAudioInput::_setSampleRate(unsigned int sampleRate)
{
  if (m_sampleRate == sampleRate || sampleRate == 0)
    return;

  destroySynth();

  m_sampleRate = sampleRate;

  m_fluidSettings = new_fluid_settings();
  fluid_settings_setnum(m_fluidSettings, "synth.gain",
                        FLUID_GLOBAL_VOLUME_GAIN);
  fluid_settings_setint(m_fluidSettings, "synth.audio-channels",
                        FLUID_AUDIO_CHANNELS_PAIR); // 1 pair of audio channels
  fluid_settings_setint(m_fluidSettings, "synth.lock-memory", 0);
  fluid_settings_setint(m_fluidSettings, "synth.threadsafe-api", 0);
  fluid_settings_setint(m_fluidSettings, "synth.midi-channels", 16);
  fluid_settings_setint(m_fluidSettings, "synth.dynamic-sample-loading", 1);
  fluid_settings_setint(m_fluidSettings, "synth.polyphony", 512);
  fluid_settings_setint(m_fluidSettings, "synth.min-note-length",
                        MIN_NOTE_LENGTH);
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
  constexpr auto channelIdx =
      0; // At the moment, only one staff (or instrument) at a time is
         // supported, hence just one channel.
  fluid_synth_set_interp_method(m_fluidSynth, channelIdx, FLUID_INTERP_DEFAULT);
  fluid_synth_pitch_wheel_sens(m_fluidSynth, channelIdx, 24);

  // The following will become relevant when we allow the user to change the
  // sound. At the moment we just use piano.
  fluid_synth_bank_select(m_fluidSynth, channelIdx, 0);
  fluid_synth_program_change(m_fluidSynth, channelIdx, 0);

  fluid_synth_cc(m_fluidSynth, channelIdx, 7, DEFAULT_MIDI_VOLUME);
  fluid_synth_cc(m_fluidSynth, channelIdx, 74, 0);
  fluid_synth_set_portamento_mode(m_fluidSynth, channelIdx,
                                  FLUID_CHANNEL_PORTAMENTO_MODE_EACH_NOTE);
  fluid_synth_set_legato_mode(m_fluidSynth, channelIdx,
                              FLUID_CHANNEL_LEGATO_MODE_RETRIGGER);
  fluid_synth_activate_tuning(m_fluidSynth, channelIdx, 0, 0, 0);
}

void FluidTrackAudioInput::destroySynth()
{
  if (m_fluidSynth)
  {
    delete_fluid_synth(m_fluidSynth);
    m_fluidSynth = nullptr;
  }
  if (m_fluidSettings)
  {
    delete_fluid_settings(m_fluidSettings);
    m_fluidSettings = nullptr;
  }
}

muse::audio::samples_t
FluidTrackAudioInput::_process(float *buffer,
                               muse::audio::samples_t samplesPerChannel)
{
  fluid_synth_write_float(m_fluidSynth, samplesPerChannel, buffer, 0,
                          FLUID_AUDIO_CHANNELS_COUNT, buffer, 1,
                          FLUID_AUDIO_CHANNELS_COUNT);
  return samplesPerChannel;
}
} // namespace dgk