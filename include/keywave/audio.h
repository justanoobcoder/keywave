#pragma once

#include <chrono>
#include <memory>
#include <string_view>

namespace keywave {

using namespace std::chrono_literals;

class AudioEngine {
  public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;

    AudioEngine(AudioEngine &&) noexcept;
    AudioEngine &operator=(AudioEngine &&) noexcept;

    [[nodiscard]] bool init();
    void shutdown();
    [[nodiscard]] bool isReady() const noexcept;

    void setVolume(float volume);

    void playClickTone(float frequencyHz = 1200.0F,
                       std::chrono::duration<float> duration = 30ms);

    void playSound(std::string_view soundFile);

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace keywave
