#include "keywave/device.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <system_error>

namespace keywave::device {

namespace {

constexpr std::size_t kBitsPerLong = 8 * sizeof(uint64_t);
constexpr std::size_t kKeyBitsWords = (KEY_MAX / kBitsPerLong) + 1;

class UniqueFd {
 public:
  explicit UniqueFd(int fd) noexcept : fd_(fd) {}
  ~UniqueFd() {
    if (fd_ >= 0) ::close(fd_);
  }

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;

  UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
      if (fd_ >= 0) ::close(fd_);
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  [[nodiscard]] int Get() const noexcept { return fd_; }
  [[nodiscard]] bool Valid() const noexcept { return fd_ >= 0; }

 private:
  int fd_ = -1;
};

[[nodiscard]] bool HasKeyCode(int fd, int keyCode) {
  std::array<uint64_t, kKeyBitsWords> keybits{};
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits.data()) < 0) {
    return false;
  }
  const std::size_t idx = static_cast<std::size_t>(keyCode) / kBitsPerLong;
  const std::size_t bit = static_cast<std::size_t>(keyCode) % kBitsPerLong;
  return ((keybits[idx] >> bit) & 1UL) != 0U;
}

[[nodiscard]] std::string GetDeviceName(int fd) {
  char name[256] = "Unknown";
  ioctl(fd, EVIOCGNAME(sizeof(name)), name);
  return name;
}

[[nodiscard]] std::string ToLower(std::string_view str) {
  std::string lower(str);
  for (char& c : lower) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return lower;
}

[[nodiscard]] int ParseEventIndex(const std::filesystem::path& path) {
  const std::string filename = path.filename().string();
  std::string_view sv = filename;
  if (!sv.starts_with("event")) {
    return -1;
  }
  sv.remove_prefix(5);
  int index = -1;
  const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), index);
  if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
    return -1;
  }
  return index;
}

[[nodiscard]] bool CompareDeviceInfo(const DeviceInfo& a, const DeviceInfo& b) {
  return ParseEventIndex(a.path) < ParseEventIndex(b.path);
}

}  // namespace

std::vector<DeviceInfo> ListInputDevices() {
  namespace fs = std::filesystem;
  std::vector<DeviceInfo> devices;

  std::error_code ec;
  if (!fs::exists("/dev/input", ec)) {
    return devices;
  }

  for (const auto& entry : fs::directory_iterator("/dev/input", ec)) {
    if (ParseEventIndex(entry.path()) < 0) {
      continue;
    }

    UniqueFd fd(open(entry.path().c_str(), O_RDONLY | O_NONBLOCK));
    if (!fd.Valid()) {
      continue;
    }

    DeviceInfo info;
    info.path = entry.path();
    info.name = GetDeviceName(fd.Get());
    info.is_keyboard = HasKeyCode(fd.Get(), KEY_A);
    info.is_mouse = HasKeyCode(fd.Get(), BTN_LEFT);

    devices.push_back(std::move(info));
  }

  std::sort(devices.begin(), devices.end(), CompareDeviceInfo);

  return devices;
}

std::optional<std::filesystem::path> FindDeviceByNameOrCapability(std::string_view name_or_path,
                                                                  int fallback_keycode) {
  namespace fs = std::filesystem;

  std::error_code ec;
  if (!name_or_path.empty() && fs::exists(name_or_path, ec)) {
    const std::string name_or_path_str(name_or_path);
    UniqueFd fd(open(name_or_path_str.c_str(), O_RDONLY | O_NONBLOCK));
    if (fd.Valid()) {
      std::cout << "Using input device by path: " << GetDeviceName(fd.Get()) << " (" << name_or_path
                << ")\n";
      return fs::path(name_or_path);
    }
    std::cerr << "Warning: Cannot open specified input device path: " << name_or_path << "\n";
  }

  const auto devices = ListInputDevices();
  const std::string target_lower = ToLower(name_or_path);

  if (!name_or_path.empty()) {
    for (const auto& dev : devices) {
      if (ToLower(dev.name).find(target_lower) != std::string::npos) {
        std::cout << "Matched input device by name \"" << name_or_path << "\": " << dev.name << " ("
                  << dev.path.string() << ")\n";
        return dev.path;
      }
    }
    std::cerr << "Warning: No input device found matching name \"" << name_or_path
              << "\". Falling back to auto-detection.\n";
  }

  for (const auto& dev : devices) {
    UniqueFd fd(open(dev.path.c_str(), O_RDONLY | O_NONBLOCK));
    if (fd.Valid() && HasKeyCode(fd.Get(), fallback_keycode)) {
      std::cout << "Auto-detected input device: " << dev.name << " (" << dev.path.string() << ")\n";
      return dev.path;
    }
  }

  return std::nullopt;
}

std::optional<std::filesystem::path> FindMouseDevice(std::string_view name_or_path) {
  return FindDeviceByNameOrCapability(name_or_path, BTN_LEFT);
}

std::optional<std::filesystem::path> FindKeyboardDevice(std::string_view name_or_path) {
  return FindDeviceByNameOrCapability(name_or_path, KEY_A);
}

}  // namespace keywave::device
