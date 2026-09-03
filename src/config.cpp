#include "keywave/config.h"

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <string>
#include <string_view>

namespace keywave {

    namespace {

        constexpr std::string_view kWhitespace = " \t\r\n";

        [[nodiscard]] constexpr std::string_view trim(std::string_view str) noexcept {
            const auto start = str.find_first_not_of(kWhitespace);
            if (start == std::string_view::npos) {
                return {};
            }
            const auto end = str.find_last_not_of(kWhitespace);
            return str.substr(start, end - start + 1);
        }

        [[nodiscard]] std::filesystem::path expandPath(std::string_view rawPath) {
            if (rawPath.empty()) {
                return {};
            }
            if (rawPath.front() == '~') {
                const char* home = std::getenv("HOME");
                if (home != nullptr) {
                    std::filesystem::path result(home);
                    if (rawPath.size() > 1 && (rawPath[1] == '/' || rawPath[1] == '\\')) {
                        result /= rawPath.substr(2);
                    }
                    return result;
                }
            }
            return std::filesystem::path(rawPath);
        }

        [[nodiscard]] std::optional<float> parseVolume(std::string_view str) {
            std::string s(trim(str));
            try {
                std::size_t idx = 0;
                const float val = std::stof(s, &idx);
                if (idx != s.size()) {
                    return std::nullopt;
                }

                if (val < 0.0F) {
                    std::cerr << "Volume cannot be negative: " << str << "\n";
                    return std::nullopt;
                }
                return val;
            } catch (const std::exception&) {
                return std::nullopt;
            }
        }

    } // namespace

    std::filesystem::path getDefaultConfigPath() {
        const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");
        if (xdgConfigHome != nullptr && *xdgConfigHome != '\0') {
            return std::filesystem::path(xdgConfigHome) / "keywave" / "keywave.conf";
        }

        const char* home = std::getenv("HOME");
        if (home != nullptr && *home != '\0') {
            return std::filesystem::path(home) / ".config" / "keywave" / "keywave.conf";
        }

        return std::filesystem::path("keywave.conf");
    }

    std::optional<Config> loadConfigFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::nullopt;
        }

        Config config;
        config.configPath = path;

        std::string line;
        std::size_t lineNum = 0;
        while (std::getline(file, line)) {
            ++lineNum;
            std::string_view sv = trim(line);
            if (sv.empty() || sv.front() == '#' || sv.front() == ';') {
                continue;
            }

            if (sv.front() == '[' && sv.back() == ']') {
                continue;
            }

            const auto eqPos = sv.find('=');
            if (eqPos == std::string_view::npos) {
                std::cerr << "Malformed line " << lineNum << " in " << path.string() << ": " << line << "\n";
                continue;
            }

            const auto key = trim(sv.substr(0, eqPos));
            const auto value = trim(sv.substr(eqPos + 1));

            if (key == "volume") {
                if (const auto vol = parseVolume(value)) {
                    config.volume = *vol;
                } else {
                    std::cerr
                        << "Invalid volume value '"
                        << value
                        << "' at line "
                        << lineNum
                        << " in "
                        << path.string()
                        << "\n";
                }
            } else if (key == "keyboard_soundpack" || key == "keyboard_pack" || key == "soundpack") {
                config.keyboardPack = expandPath(value);
            } else if (key == "mouse_sound" || key == "mouse_soundpack" || key == "mouse") {
                config.mouseSound = expandPath(value);
            } else {
                std::cerr
                    << "Unknown configuration key '"
                    << key
                    << "' at line "
                    << lineNum
                    << " in "
                    << path.string()
                    << "\n";
            }
        }

        return config;
    }

    void printUsage(std::string_view programName) {
        std::cout
            << "Usage: "
            << programName
            << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  -c, --config <path>         Path to configuration file\n"
            << "  -v, --volume <float>        Audio playback volume (0.0 - 1.0)\n"
            << "  -k, --keyboard-pack <path>  Path to keyboard soundpack directory\n"
            << "  -m, --mouse-sound <path>    Path to mouse sound audio file\n"
            << "  -h, --help                  Show this help message and exit\n\n"
            << "Default config file location: $XDG_CONFIG_HOME/keywave/keywave.conf\n";
    }

    ParseResult parseConfig(int argc, char* const argv[]) {
        std::filesystem::path customConfigPath;
        std::optional<float> cliVolume;
        std::optional<std::filesystem::path> cliKeyboardPack;
        std::optional<std::filesystem::path> cliMouseSound;

        constexpr const char* const shortOpts = "c:v:k:m:h";
        constexpr struct option longOpts[] = {
            {"config", required_argument, nullptr, 'c'},
            {"volume", required_argument, nullptr, 'v'},
            {"keyboard-pack", required_argument, nullptr, 'k'},
            {"mouse-sound", required_argument, nullptr, 'm'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
        };

        optind = 1;
        int opt = 0;
        while ((opt = getopt_long(argc, argv, shortOpts, longOpts, nullptr)) != -1) {
            switch (opt) {
            case 'c':
                customConfigPath = expandPath(optarg);
                break;
            case 'v': {
                const auto vol = parseVolume(optarg);
                if (!vol) {
                    std::cerr << "Error: Invalid volume specified on CLI: " << optarg << "\n";
                    return ParseResult{ParseStatus::Error, {}};
                }
                cliVolume = vol;
                break;
            }
            case 'k':
                cliKeyboardPack = expandPath(optarg);
                break;
            case 'm':
                cliMouseSound = expandPath(optarg);
                break;
            case 'h':
                printUsage(argv[0]);
                return ParseResult{ParseStatus::HelpRequested, {}};
            default:
                printUsage(argv[0]);
                return ParseResult{ParseStatus::Error, {}};
            }
        }

        const std::filesystem::path configPathToLoad =
            !customConfigPath.empty() ? customConfigPath : getDefaultConfigPath();

        Config config;
        if (std::filesystem::exists(configPathToLoad)) {
            if (auto loaded = loadConfigFile(configPathToLoad)) {
                config = std::move(*loaded);
                std::cout << "Loaded configuration from " << configPathToLoad.string() << "\n";
            } else {
                std::cerr << "Warning: Failed to read config file " << configPathToLoad.string() << "\n";
            }
        } else if (!customConfigPath.empty()) {
            std::cerr << "Error: Specified config file does not exist: " << customConfigPath.string() << "\n";
            return ParseResult{ParseStatus::Error, {}};
        }

        if (cliVolume) {
            config.volume = *cliVolume;
        }
        if (cliKeyboardPack) {
            config.keyboardPack = std::move(*cliKeyboardPack);
        }
        if (cliMouseSound) {
            config.mouseSound = std::move(*cliMouseSound);
        }

        return ParseResult{ParseStatus::Success, std::move(config)};
    }

} // namespace keywave
