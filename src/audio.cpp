#define MA_IMPLEMENTATION

#include "keywave/audio.h"

#include "miniaudio.h"

#include <filesystem>
#include <iostream>

namespace keywave {

    struct AudioEngine::Impl {
        ma_engine engine{};
        bool ready{false};

        ~Impl() {
            if (ready) {
                ma_engine_uninit(&engine);
                ready = false;
            }
        }
    };

    AudioEngine::AudioEngine() : m_impl(std::make_unique<Impl>()) {}

    AudioEngine::~AudioEngine() = default;

    AudioEngine::AudioEngine(AudioEngine&&) noexcept = default;
    AudioEngine& AudioEngine::operator=(AudioEngine&&) noexcept = default;

    bool AudioEngine::init() {
        if (m_impl->ready) {
            return true;
        }

        ma_result result = ma_engine_init(nullptr, &m_impl->engine);
        m_impl->ready = (result == MA_SUCCESS);
        return m_impl->ready;
    }

    void AudioEngine::shutdown() {
        if (m_impl->ready) {
            ma_engine_uninit(&m_impl->engine);
            m_impl->ready = false;
        }
    }

    bool AudioEngine::isReady() const noexcept { return m_impl->ready; }

    void AudioEngine::setVolume(float volume) {
        if (m_impl->ready) {
            ma_engine_set_volume(&m_impl->engine, volume);
        }
    }

    void AudioEngine::playClickTone(float frequencyHz, std::chrono::duration<float> duration) {
        (void)frequencyHz;
        (void)duration;
        std::cerr << "[AudioEngine] No playable sound available for this click.\n";
    }

    void AudioEngine::playSound(std::string_view soundFile) {
        if (!m_impl->ready) {
            return;
        }

        std::filesystem::path path{soundFile};

        if (soundFile.empty() || !std::filesystem::exists(path)) {
            playClickTone();
            return;
        }

        if (ma_engine_play_sound(&m_impl->engine, path.c_str(), nullptr) != MA_SUCCESS) {
            std::cerr << "[AudioEngine] Failed to play sound: " << soundFile << "\n";
        }
    }

} // namespace keywave
