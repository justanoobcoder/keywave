#define MA_IMPLEMENTATION

#include "keywave/audio.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>

#include "miniaudio.h"

namespace keywave {

struct AudioEngine::Impl {
  ma_engine engine{};
  ma_sound_group key_group{};
  ma_sound_group mouse_group{};
  bool ready{false};
  std::unordered_set<std::string> preloaded_sounds{};

  void UnloadAll() {
    if (ready) {
      ma_resource_manager* rm = ma_engine_get_resource_manager(&engine);
      if (rm != nullptr) {
        for (const auto& path : preloaded_sounds) {
          ma_resource_manager_unregister_file(rm, path.c_str());
        }
      }
      preloaded_sounds.clear();
    }
  }

  ~Impl() {
    if (ready) {
      UnloadAll();
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
    impl_->UnloadAll();
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

bool AudioEngine::PreloadSound(std::string_view file) {
  if (!impl_->ready || file.empty()) {
    return false;
  }

  const std::string file_str(file);
  if (impl_->preloaded_sounds.contains(file_str)) {
    return true;
  }

  const std::filesystem::path path{file_str};
  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    return false;
  }

  ma_resource_manager* rm = ma_engine_get_resource_manager(&impl_->engine);
  if (rm == nullptr) {
    return false;
  }

  // Register file in memory buffer (encoded) to eliminate disk I/O without bloating RAM with raw
  // PCM arrays
  const ma_result result = ma_resource_manager_register_file(rm, file_str.c_str(), 0);
  if (result == MA_SUCCESS || result == MA_ALREADY_EXISTS) {
    impl_->preloaded_sounds.insert(file_str);
    return true;
  }

  return false;
}

void AudioEngine::PlaySound(std::string_view file, SoundChannel channel) {
  if (!impl_->ready || file.empty()) {
    return;
  }

  const std::string file_str(file);

  if (!impl_->preloaded_sounds.contains(file_str)) {
    if (!PreloadSound(file_str)) {
      std::cerr << "[AudioEngine] Sound file not found or invalid: " << file_str << "\n";
      PlayClickTone();
      return;
    }
  }

  ma_sound_group* group =
    (channel == SoundChannel::kKeyboard) ? &impl_->key_group : &impl_->mouse_group;

  if (ma_engine_play_sound(&impl_->engine, file_str.c_str(), group) != MA_SUCCESS) {
    std::cerr << "[AudioEngine] Failed to play sound: " << file_str << "\n";
  }
}

}  // namespace keywave
