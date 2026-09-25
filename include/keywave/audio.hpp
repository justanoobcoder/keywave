#ifndef KEYWAVE_AUDIO_HPP
#define KEYWAVE_AUDIO_HPP

#include <chrono>
#include <memory>
#include <string_view>

namespace keywave {

using namespace std::chrono_literals;

enum class SoundChannel { kKeyboard, kMouse };

class AudioEngine {
 public:
  AudioEngine();

  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  AudioEngine(AudioEngine&&) noexcept;
  AudioEngine& operator=(AudioEngine&&) noexcept;

  [[nodiscard]] bool Init();
  void Shutdown();
  [[nodiscard]] bool IsReady() const noexcept;

  void SetVolume(float volume);
  void SetChannelVolume(SoundChannel channel, float volume);

  void PlayClickTone(float frequencyHz = 1200.0F,
                     std::chrono::duration<float> duration = 30ms);

  void PlaySound(std::string_view soundFile,
                 SoundChannel channel = SoundChannel::kKeyboard);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace keywave

#endif
