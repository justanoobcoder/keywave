#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

namespace keywave {

struct Config {
    float volume{1.0F};
    std::filesystem::path keyboardPack{
        "assets/sounds/keyboards/cherrymx-red-abs"};
    std::filesystem::path mouseSound{"assets/sounds/mouses/mouse-click.mp3"};
    std::filesystem::path configPath;
};

[[nodiscard]] std::filesystem::path getDefaultConfigPath();

[[nodiscard]] std::optional<Config>
loadConfigFile(const std::filesystem::path &path);

enum class ParseStatus { Success, HelpRequested, Error };

struct ParseResult {
    ParseStatus status{ParseStatus::Success};
    Config config{};
};

[[nodiscard]] ParseResult parseConfig(int argc, char *const argv[]);

void printUsage(std::string_view programName);

} // namespace keywave
