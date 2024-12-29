#pragma once

namespace dgk
{
class IOrchestrionSynthesizer
{
public:
  virtual ~IOrchestrionSynthesizer() = default;

  virtual int sampleRate() const = 0;
  virtual int numChannels() const = 0;
  virtual size_t process(float *buffer, size_t samplesPerChannel) = 0;
  virtual void onNoteOns(size_t numNoteons, const int *pitches,
                         const float *velocities) = 0;
  virtual void onNoteOffs(size_t numNoteoffs, const int *pitches) = 0;
  virtual void onPedal(bool on) = 0;
};
} // namespace dgk
