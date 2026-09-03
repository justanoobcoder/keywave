#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace keywave {

struct DeviceInfo {
    std::string name;
    std::filesystem::path path;
    bool isKeyboard{false};
    bool isMouse{false};
};

[[nodiscard]] std::vector<DeviceInfo> listInputDevices();

[[nodiscard]] std::optional<std::filesystem::path>
findDeviceByNameOrCapability(std::string_view nameOrPath, int fallbackKeyCode);

[[nodiscard]] std::optional<std::filesystem::path>
findMouseDevice(std::string_view preferredNameOrPath = "");

[[nodiscard]] std::optional<std::filesystem::path>
findKeyboardDevice(std::string_view preferredNameOrPath = "");

} // namespace keywave
