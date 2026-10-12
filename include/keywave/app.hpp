#ifndef KEYWAVE_APP_HPP
#define KEYWAVE_APP_HPP

#include <atomic>
#include <filesystem>

#include "keywave/audio.hpp"
#include "keywave/config.hpp"
#include "keywave/soundpack.hpp"

namespace keywave {

class App {
 public:
  void PrintDeviceList();
  void RunMouseListener(const std::filesystem::path& device,
                        const std::filesystem::path& sound_path, AudioEngine& audio);

  void RunKeyboardListener(const std::filesystem::path& device, const SoundPack& soundpack,
                           AudioEngine& audio);

  bool ConfigureAudio(const Config& config, AudioEngine& audio);
  void PrintBanner(const Config& config);
  int Run(const Config& config);

 private:
  static inline std::atomic<bool> running_{true};
};

}  // namespace keywave

#endif  // KEYWAVE_APP_HPP
