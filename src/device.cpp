#include "keywave/device.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>

namespace keywave {

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
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return lower;
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
    const std::string filename = entry.path().filename().string();
    if (filename.rfind("event", 0) != 0) {
      continue;
    }

    UniqueFd fd(open(entry.path().c_str(), O_RDONLY | O_NONBLOCK));
    if (!fd.Valid()) {
      continue;
    }

    DeviceInfo info;
    info.path = entry.path();
    info.name = GetDeviceName(fd.Get());
    info.isKeyboard = HasKeyCode(fd.Get(), KEY_A);
    info.isMouse = HasKeyCode(fd.Get(), BTN_LEFT);

    devices.push_back(std::move(info));
  }

  std::sort(devices.begin(), devices.end(),
            [](const DeviceInfo& a, const DeviceInfo& b) {
              auto get_index = [](const std::filesystem::path& p) {
                std::string filename = p.filename().string();
                if (filename.rfind("event", 0) == 0) {
                  try {
                    return std::stoi(filename.substr(5));
                  } catch (...) {
                  }
                }
                return -1;
              };
              return get_index(a.path) < get_index(b.path);
            });

  return devices;
}

std::optional<std::filesystem::path> FindDeviceByNameOrCapability(
  std::string_view nameOrPath, int fallbackKeyCode) {
  namespace fs = std::filesystem;

  if (!nameOrPath.empty() && fs::exists(nameOrPath)) {
    UniqueFd fd(open(nameOrPath.data(), O_RDONLY | O_NONBLOCK));
    if (fd.Valid()) {
      std::cout << "Using input device by path: " << GetDeviceName(fd.Get())
                << " (" << nameOrPath << ")\n";
      return fs::path(nameOrPath);
    }
    std::cerr << "Warning: Cannot open specified input device path: "
              << nameOrPath << "\n";
  }

  const auto devices = ListInputDevices();
  const std::string target_lower = ToLower(nameOrPath);

  if (!nameOrPath.empty()) {
    for (const auto& dev : devices) {
      if (ToLower(dev.name).find(target_lower) != std::string::npos) {
        std::cout << "Matched input device by name \"" << nameOrPath
                  << "\": " << dev.name << " (" << dev.path.string() << ")\n";
        return dev.path;
      }
    }
    std::cerr << "Warning: No input device found matching name \"" << nameOrPath
              << "\". Falling back to auto-detection.\n";
  }

  for (const auto& dev : devices) {
    UniqueFd fd(open(dev.path.c_str(), O_RDONLY | O_NONBLOCK));
    if (fd.Valid() && HasKeyCode(fd.Get(), fallbackKeyCode)) {
      std::cout << "Auto-detected input device: " << dev.name << " ("
                << dev.path.string() << ")\n";
      return dev.path;
    }
  }

  return std::nullopt;
}

std::optional<std::filesystem::path> FindMouseDevice(
  std::string_view preferredNameOrPath) {
  return FindDeviceByNameOrCapability(preferredNameOrPath, BTN_LEFT);
}

std::optional<std::filesystem::path> FindKeyboardDevice(
  std::string_view preferredNameOrPath) {
  return FindDeviceByNameOrCapability(preferredNameOrPath, KEY_A);
}

}  // namespace keywave
