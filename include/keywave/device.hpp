#ifndef KEYWAVE_DEVICE_HPP
#define KEYWAVE_DEVICE_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace keywave::device {

struct DeviceInfo {
  std::string name{};
  std::filesystem::path path{};
  bool is_keyboard{false};
  bool is_mouse{false};
};

[[nodiscard]] std::vector<DeviceInfo> ListInputDevices();
[[nodiscard]] std::optional<std::filesystem::path> FindDeviceByNameOrCapability(
  std::string_view name_or_path, int fallback_keycode);

[[nodiscard]] std::optional<std::filesystem::path> FindMouseDevice(
  std::string_view name_or_path = "");

[[nodiscard]] std::optional<std::filesystem::path> FindKeyboardDevice(
  std::string_view name_or_path = "");

}  // namespace keywave::device
#endif
