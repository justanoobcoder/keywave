#ifndef KEYWAVE_AUDIO_HPP
#define KEYWAVE_AUDIO_HPP

#include <chrono>
#include <memory>
#include <string_view>

#include "miniaudio.h"

namespace keywave {

enum class SoundChannel { kKeyboard, kMouse };

class AudioEngine {
 public:
  AudioEngine();

  ~AudioEngine() = default;

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  AudioEngine(AudioEngine&&) noexcept = default;
  AudioEngine& operator=(AudioEngine&&) noexcept = default;

  [[nodiscard]] bool Init();
  void Shutdown();
  [[nodiscard]] bool IsReady() const noexcept { return impl_->ready; };

  void SetVolume(float volume);
  void SetChannelVolume(SoundChannel channel, float volume);

  void PlayClickTone(float freq_hz = 1200.0F,
                     std::chrono::duration<float> duration = std::chrono::milliseconds(30));

  void PlaySound(std::string_view file, SoundChannel channel = SoundChannel::kKeyboard);

 private:
  struct Impl {
    ma_engine engine{};
    ma_sound_group keyboardGroup{};
    ma_sound_group mouseGroup{};
    bool ready{false};

    ~Impl() {
      if (ready) {
        ma_sound_group_uninit(&mouseGroup);
        ma_sound_group_uninit(&keyboardGroup);
        ma_engine_uninit(&engine);
        ready = false;
      }
    }
  };

  std::unique_ptr<Impl> impl_;
};

}  // namespace keywave

#endif
