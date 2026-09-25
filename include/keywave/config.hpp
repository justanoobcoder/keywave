#ifndef KEYWAVE_CONFIG_HPP
#define KEYWAVE_CONFIG_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace keywave {

struct Config {
  float volume{1.0F};
  std::optional<float> keyboardVolume;
  std::optional<float> mouseVolume;
  std::filesystem::path keyboardPack{
    "assets/sounds/keyboards/cherrymx-red-abs"};
  std::filesystem::path mouseSound{"assets/sounds/mouses/mouse-click.mp3"};
  std::string keyboardDevice;
  std::string mouseDevice;
  std::filesystem::path configPath;
};

[[nodiscard]] std::filesystem::path GetDefaultConfigPath();

[[nodiscard]] std::optional<Config> LoadConfigFile(
  const std::filesystem::path& path);

enum class ParseStatus {
  kSuccess,
  kHelpRequested,
  kVersionRequested,
  kListDevicesRequested,
  kError
};

struct ParseResult {
  ParseStatus status{ParseStatus::kSuccess};
  Config config{};
};

[[nodiscard]] ParseResult ParseConfig(int argc, char* const argv[]);

void PrintUsage(std::string_view programName);
void PrintVersion();

}  // namespace keywave

#endif
