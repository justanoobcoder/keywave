#include "keywave/config.hpp"

#include <getopt.h>

#include <cstdlib>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

#include "keywave/version.hpp"

using std::filesystem::path;

namespace keywave {

namespace {

constexpr std::string_view kWhitespace = " \t\r\n";

[[nodiscard]] constexpr std::string_view Trim(std::string_view str) noexcept {
  const auto start = str.find_first_not_of(kWhitespace);
  if (start == std::string_view::npos) {
    return {};
  }
  const auto end = str.find_last_not_of(kWhitespace);
  return str.substr(start, end - start + 1);
}

[[nodiscard]] path ExpandPath(std::string_view rawPath) {
  if (rawPath.empty()) {
    return {};
  }
  if (rawPath.front() == '~') {
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
      path result(home);
      if (rawPath.size() > 1 && (rawPath[1] == '/' || rawPath[1] == '\\')) {
        result /= rawPath.substr(2);
      }
      return result;
    }
  }
  return rawPath;
}

[[nodiscard]] std::optional<float> ParseVolume(std::string_view str) {
  std::string_view s = Trim(str);
  if (s.empty()) {
    return std::nullopt;
  }
  if (s.front() == '+') {
    s.remove_prefix(1);
  }
  if (s.empty()) {
    return std::nullopt;
  }

  float val = 0.0F;
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc{} || ptr != s.data() + s.size()) {
    return std::nullopt;
  }

  if (val < 0.0F) {
    std::cerr << "Volume cannot be negative: " << str << "\n";
    return std::nullopt;
  }
  return val;
}

}  // namespace

path GetDefaultConfigPath() {
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  if (xdg_config_home != nullptr && *xdg_config_home != '\0') {
    return path(xdg_config_home) / "keywave" / "keywave.conf";
  }

  const char* home = std::getenv("HOME");
  if (home != nullptr && *home != '\0') {
    return path(home) / ".config" / "keywave" / "keywave.conf";
  }

  return "keywave.conf";
}

std::optional<Config> LoadConfigFile(const path& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  Config config;
  config.configPath = path;

  std::string line;
  std::size_t line_num = 0;
  while (std::getline(file, line)) {
    ++line_num;
    std::string_view sv = Trim(line);
    if (sv.empty() || sv.front() == '#' || sv.front() == ';') {
      continue;
    }

    if (sv.front() == '[' && sv.back() == ']') {
      continue;
    }

    const auto eq_pos = sv.find('=');
    if (eq_pos == std::string_view::npos) {
      std::cerr << "Malformed line " << line_num << " in " << path.string() << ": " << line << "\n";
      continue;
    }

    const auto key = Trim(sv.substr(0, eq_pos));
    const auto value = Trim(sv.substr(eq_pos + 1));

    if (key == "volume") {
      if (const auto vol = ParseVolume(value)) {
        config.volume = *vol;
      } else {
        std::cerr << "Invalid volume value '" << value << "' at line " << line_num << " in "
                  << path.string() << "\n";
      }
    } else if (key == "keyboard_volume" || key == "keyboard_vol") {
      if (const auto vol = ParseVolume(value)) {
        config.keyboardVolume = vol;
      } else {
        std::cerr << "Invalid keyboard volume value '" << value << "' at line " << line_num
                  << " in " << path.string() << "\n";
      }
    } else if (key == "mouse_volume" || key == "mouse_vol") {
      if (const auto vol = ParseVolume(value)) {
        config.mouseVolume = vol;
      } else {
        std::cerr << "Invalid mouse volume value '" << value << "' at line " << line_num << " in "
                  << path.string() << "\n";
      }
    } else if (key == "keyboard_soundpack" || key == "keyboard_pack" || key == "soundpack") {
      config.keyboardPack = ExpandPath(value);
    } else if (key == "mouse_sound" || key == "mouse_soundpack" || key == "mouse") {
      config.mouseSound = ExpandPath(value);
    } else if (key == "keyboard_device" || key == "keyboard_dev" || key == "keyboard") {
      config.keyboardDevice = std::string(value);
    } else if (key == "mouse_device" || key == "mouse_dev") {
      config.mouseDevice = std::string(value);
    } else {
      std::cerr << "Unknown configuration key '" << key << "' at line " << line_num << " in "
                << path.string() << "\n";
    }
  }

  return config;
}

void PrintUsage(std::string_view programName) {
  std::cout << "Usage: " << programName << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  -c, --config <path>             Path to configuration file\n"
            << "  -v, --volume <float>            Master audio playback volume (e.g. "
               "0.8 or 1.0)\n"
            << "      --keyboard-vol <float>      Keyboard audio playback volume "
               "override\n"
            << "      --mouse-vol <float>         Mouse audio playback volume "
               "override\n"
            << "  -k, --keyboard-pack <path>      Path to keyboard soundpack "
               "directory\n"
            << "  -m, --mouse-sound <path>        Path to mouse sound audio file\n"
            << "  -K, --keyboard-dev <name|path>  Keyboard device name (or /dev/input "
               "path)\n"
            << "  -M, --mouse-dev <name|path>     Mouse device name (or /dev/input "
               "path)\n"
            << "  -l, --list-devices              List available input devices and "
               "exit\n"
            << "  -V, --version                   Show version information and exit\n"
            << "  -h, --help                      Show this help message and exit\n\n"
            << "Default config file location: $XDG_CONFIG_HOME/keywave/keywave.conf\n";
}

void PrintVersion() {
  std::cout << "keywave v" << kAppVersion << " (build: " << kGitCommit << ", " << kBuildDate
            << ")\n";
}

ParseResult ParseConfig(int argc, char* const argv[]) {
  path custom_config_path;
  std::optional<float> cli_volume;
  std::optional<float> cli_keyboard_volume;
  std::optional<float> cli_mouse_volume;
  std::optional<path> cli_keyboard_pack;
  std::optional<path> cli_mouse_sound;
  std::optional<std::string> cli_keyboard_device;
  std::optional<std::string> cli_mouse_device;

  constexpr const char* const kShortOpts = "c:v:k:m:K:M:lVh";
  enum LongOnlyOpt { kOptKeyboardVol = 1000, kOptMouseVol };
  constexpr struct option kLongOpts[] = {
    {"config", required_argument, nullptr, 'c'},
    {"volume", required_argument, nullptr, 'v'},
    {"keyboard-vol", required_argument, nullptr, kOptKeyboardVol},
    {"keyboard-volume", required_argument, nullptr, kOptKeyboardVol},
    {"mouse-vol", required_argument, nullptr, kOptMouseVol},
    {"mouse-volume", required_argument, nullptr, kOptMouseVol},
    {"keyboard-pack", required_argument, nullptr, 'k'},
    {"mouse-sound", required_argument, nullptr, 'm'},
    {"keyboard-dev", required_argument, nullptr, 'K'},
    {"mouse-dev", required_argument, nullptr, 'M'},
    {"list-devices", no_argument, nullptr, 'l'},
    {"version", no_argument, nullptr, 'V'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0}};

  optind = 1;
  int opt = 0;
  while ((opt = getopt_long(argc, argv, kShortOpts, kLongOpts, nullptr)) != -1) {
    switch (opt) {
      case 'c':
        custom_config_path = ExpandPath(optarg);
        break;
      case 'v': {
        const auto vol = ParseVolume(optarg);
        if (!vol) {
          std::cerr << "Error: Invalid volume specified on CLI: " << optarg << "\n";
          return ParseResult{ParseStatus::kError, {}};
        }
        cli_volume = vol;
        break;
      }
      case kOptKeyboardVol: {
        const auto vol = ParseVolume(optarg);
        if (!vol) {
          std::cerr << "Error: Invalid keyboard volume specified on CLI: " << optarg << "\n";
          return ParseResult{ParseStatus::kError, {}};
        }
        cli_keyboard_volume = vol;
        break;
      }
      case kOptMouseVol: {
        const auto vol = ParseVolume(optarg);
        if (!vol) {
          std::cerr << "Error: Invalid mouse volume specified on CLI: " << optarg << "\n";
          return ParseResult{ParseStatus::kError, {}};
        }
        cli_mouse_volume = vol;
        break;
      }
      case 'k':
        cli_keyboard_pack = ExpandPath(optarg);
        break;
      case 'm':
        cli_mouse_sound = ExpandPath(optarg);
        break;
      case 'K':
        cli_keyboard_device = optarg;
        break;
      case 'M':
        cli_mouse_device = optarg;
        break;
      case 'l':
        return ParseResult{ParseStatus::kListDevicesRequested, {}};
      case 'V':
        PrintVersion();
        return ParseResult{ParseStatus::kVersionRequested, {}};
      case 'h':
        PrintUsage(argv[0]);
        return ParseResult{ParseStatus::kHelpRequested, {}};
      default:
        PrintUsage(argv[0]);
        return ParseResult{ParseStatus::kError, {}};
    }
  }

  const path config_path_to_load =
    !custom_config_path.empty() ? custom_config_path : GetDefaultConfigPath();

  Config config;
  std::error_code ec;
  if (std::filesystem::exists(config_path_to_load, ec)) {
    if (auto loaded = LoadConfigFile(config_path_to_load)) {
      config = std::move(*loaded);
      std::cout << "Loaded configuration from " << config_path_to_load.string() << "\n";
    } else {
      std::cerr << "Warning: Failed to read config file " << config_path_to_load.string() << "\n";
    }
  } else if (!custom_config_path.empty()) {
    std::cerr << "Error: Specified config file does not exist: " << custom_config_path.string()
              << "\n";
    return ParseResult{ParseStatus::kError, {}};
  }

  if (cli_volume) {
    config.volume = *cli_volume;
  }
  if (cli_keyboard_volume) {
    config.keyboardVolume = cli_keyboard_volume;
  }
  if (cli_mouse_volume) {
    config.mouseVolume = cli_mouse_volume;
  }
  if (cli_keyboard_pack) {
    config.keyboardPack = std::move(*cli_keyboard_pack);
  }
  if (cli_mouse_sound) {
    config.mouseSound = std::move(*cli_mouse_sound);
  }
  if (cli_keyboard_device) {
    config.keyboardDevice = std::move(*cli_keyboard_device);
  }
  if (cli_mouse_device) {
    config.mouseDevice = std::move(*cli_mouse_device);
  }

  return ParseResult{ParseStatus::kSuccess, std::move(config)};
}

}  // namespace keywave
