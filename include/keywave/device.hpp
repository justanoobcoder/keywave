#ifndef KEYWAVE_DEVICE_HPP
#define KEYWAVE_DEVICE_HPP

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

[[nodiscard]] std::vector<DeviceInfo> ListInputDevices();

[[nodiscard]] std::optional<std::filesystem::path> FindDeviceByNameOrCapability(
  std::string_view nameOrPath, int fallbackKeyCode);

[[nodiscard]] std::optional<std::filesystem::path> FindMouseDevice(
  std::string_view preferredNameOrPath = "");

[[nodiscard]] std::optional<std::filesystem::path> FindKeyboardDevice(
  std::string_view preferredNameOrPath = "");

}  // namespace keywave

#endif
