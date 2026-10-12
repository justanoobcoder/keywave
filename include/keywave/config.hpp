#ifndef KEYWAVE_CONFIG_HPP
#define KEYWAVE_CONFIG_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace keywave {

enum class ParseStatus {
  kSuccess,
  kHelpRequested,
  kVersionRequested,
  kListDevicesRequested,
  kError,
};

class Config {
 public:
  Config() = default;
  ~Config() = default;

  void SetVolume(float volume) { vol_ = volume; }
  [[nodiscard]] constexpr float GetVolume() const { return vol_; }
  [[nodiscard]] bool IsKeyboardVolumSet() const { return key_vol_.has_value(); }
  void SetKeyboardVolume(float volume) { key_vol_ = volume; }
  [[nodiscard]] float GetKeyboardVolume() const { return key_vol_.value_or(vol_); }
  [[nodiscard]] bool IsMouseVolumSet() const { return mouse_vol_.has_value(); }
  void SetMouseVolume(float volume) { mouse_vol_ = volume; }
  [[nodiscard]] float GetMouseVolume() const { return mouse_vol_.value_or(vol_); }
  void SetKeyboardPack(const std::filesystem::path& pack_path) { key_pack_ = pack_path; }
  [[nodiscard]] const std::filesystem::path& GetKeyboardPack() const { return key_pack_; }
  void SetMouseSound(const std::filesystem::path& sound_path) { mouse_sound_ = sound_path; }
  [[nodiscard]] const std::filesystem::path& GetMouseSound() const { return mouse_sound_; }
  void SetKeyboardDevice(const std::string& dev) { key_dev_ = dev; }
  [[nodiscard]] const std::string& GetKeyboardDevice() const { return key_dev_; }
  void SetMouseDevice(const std::string& dev) { mouse_dev_ = dev; }
  [[nodiscard]] const std::string& GetMouseDevice() const { return mouse_dev_; }
  void SetConfigPath(const std::filesystem::path& path) { cfg_path_ = path; }
  [[nodiscard]] const std::filesystem::path& GetConfigPath() const { return cfg_path_; }

  [[nodiscard]] std::filesystem::path GetDefaultConfigPath();
  [[nodiscard]] std::optional<Config> LoadConfigFile(const std::filesystem::path& path);
  static void PrintUsage(std::string_view name);
  static void PrintVersion();

 private:
  float vol_{1.0F};
  std::optional<float> key_vol_;
  std::optional<float> mouse_vol_;
  std::filesystem::path key_pack_{"assets/sounds/keyboards/cherrymx-red-abs"};
  std::filesystem::path mouse_sound_{"assets/sounds/mouses/mouse-click.mp3"};
  std::string key_dev_;
  std::string mouse_dev_;
  std::filesystem::path cfg_path_;
};

struct ParseResult {
  ParseStatus status{ParseStatus::kSuccess};
  Config config{};
};

[[nodiscard]] ParseResult ParseConfig(int argc, char* const argv[]);

}  // namespace keywave

#endif
