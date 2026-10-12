#define MA_IMPLEMENTATION

#include "keywave/audio.hpp"

#include <filesystem>
#include <iostream>

#include "miniaudio.h"

namespace keywave {

struct AudioEngine::Impl {
  ma_engine engine{};
  ma_sound_group key_group{};
  ma_sound_group mouse_group{};
  bool ready{false};

  ~Impl() {
    if (ready) {
      ma_sound_group_uninit(&mouse_group);
      ma_sound_group_uninit(&key_group);
      ma_engine_uninit(&engine);
      ready = false;
    }
  }
};

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() = default;

bool AudioEngine::Init() {
  if (impl_->ready) {
    return true;
  }

  ma_result result = ma_engine_init(nullptr, &impl_->engine);
  if (result != MA_SUCCESS) {
    return false;
  }

  result = ma_sound_group_init(&impl_->engine, 0, nullptr, &impl_->key_group);
  if (result != MA_SUCCESS) {
    ma_engine_uninit(&impl_->engine);
    return false;
  }

  result = ma_sound_group_init(&impl_->engine, 0, nullptr, &impl_->mouse_group);
  if (result != MA_SUCCESS) {
    ma_sound_group_uninit(&impl_->key_group);
    ma_engine_uninit(&impl_->engine);
    return false;
  }

  impl_->ready = true;
  return true;
}

void AudioEngine::Shutdown() {
  if (impl_->ready) {
    ma_sound_group_uninit(&impl_->mouse_group);
    ma_sound_group_uninit(&impl_->key_group);
    ma_engine_uninit(&impl_->engine);
    impl_->ready = false;
  }
}

bool AudioEngine::IsReady() const noexcept { return impl_ && impl_->ready; }

void AudioEngine::SetVolume(float volume) {
  if (impl_->ready) {
    ma_engine_set_volume(&impl_->engine, volume);
  }
}

void AudioEngine::SetChannelVolume(SoundChannel channel, float volume) {
  if (!impl_->ready) {
    return;
  }
  if (channel == SoundChannel::kKeyboard) {
    ma_sound_group_set_volume(&impl_->key_group, volume);
  } else if (channel == SoundChannel::kMouse) {
    ma_sound_group_set_volume(&impl_->mouse_group, volume);
  }
}

// TODO: play a default sound
void AudioEngine::PlayClickTone(float, std::chrono::duration<float>) {
  std::cerr << "[AudioEngine] No playable sound available for this click.\n";
}

void AudioEngine::PlaySound(std::string_view file, SoundChannel channel) {
  if (!impl_->ready) {
    return;
  }

  const std::filesystem::path path{file};
  std::error_code ec;
  if (file.empty() || !std::filesystem::exists(path, ec)) {
    std::cerr << "[AudioEngine] Sound file not found: " << path.string() << "\n";
    PlayClickTone();
    return;
  }

  ma_sound_group* group =
    (channel == SoundChannel::kKeyboard) ? &impl_->key_group : &impl_->mouse_group;

  if (ma_engine_play_sound(&impl_->engine, path.c_str(), group) != MA_SUCCESS) {
    std::cerr << "[AudioEngine] Failed to play sound: " << file << "\n";
  }
}

}  // namespace keywave
