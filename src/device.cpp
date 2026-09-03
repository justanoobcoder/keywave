#include "keywave/device.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace keywave {

    namespace {

        constexpr std::size_t kBitsPerLong = 8 * sizeof(unsigned long);
        constexpr std::size_t kKeyBitsWords = (KEY_MAX / kBitsPerLong) + 1;

        class UniqueFd {
          public:
            explicit UniqueFd(int fd) noexcept : m_fd(fd) {}
            ~UniqueFd() {
                if (m_fd >= 0)
                    ::close(m_fd);
            }

            UniqueFd(const UniqueFd&) = delete;
            UniqueFd& operator=(const UniqueFd&) = delete;

            UniqueFd(UniqueFd&& other) noexcept : m_fd(other.m_fd) { other.m_fd = -1; }
            UniqueFd& operator=(UniqueFd&& other) noexcept {
                if (this != &other) {
                    if (m_fd >= 0)
                        ::close(m_fd);
                    m_fd = other.m_fd;
                    other.m_fd = -1;
                }
                return *this;
            }

            [[nodiscard]] int get() const noexcept { return m_fd; }
            [[nodiscard]] bool valid() const noexcept { return m_fd >= 0; }

          private:
            int m_fd = -1;
        };

        [[nodiscard]] bool hasKeyCode(int fd, int keyCode) {
            std::array<unsigned long, kKeyBitsWords> keybits{};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits.data()) < 0) {
                return false;
            }
            const std::size_t idx = static_cast<std::size_t>(keyCode) / kBitsPerLong;
            const std::size_t bit = static_cast<std::size_t>(keyCode) % kBitsPerLong;
            return (keybits[idx] >> bit) & 1UL;
        }

        [[nodiscard]] std::string getDeviceName(int fd) {
            char name[256] = "Unknown";
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            return name;
        }

        [[nodiscard]] std::string toLower(std::string_view str) {
            std::string lower(str);
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
            return lower;
        }

    } // namespace

    std::vector<DeviceInfo> listInputDevices() {
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
            if (!fd.valid()) {
                continue;
            }

            DeviceInfo info;
            info.path = entry.path();
            info.name = getDeviceName(fd.get());
            info.isKeyboard = hasKeyCode(fd.get(), KEY_A);
            info.isMouse = hasKeyCode(fd.get(), BTN_LEFT);

            devices.push_back(std::move(info));
        }

        std::sort(devices.begin(), devices.end(), [](const DeviceInfo& a, const DeviceInfo& b) {
            auto getIndex = [](const std::filesystem::path& p) {
                std::string filename = p.filename().string();
                if (filename.rfind("event", 0) == 0) {
                    try {
                        return std::stoi(filename.substr(5));
                    } catch (...) {
                    }
                }
                return -1;
            };
            return getIndex(a.path) < getIndex(b.path);
        });

        return devices;
    }

    std::optional<std::filesystem::path>
    findDeviceByNameOrCapability(std::string_view nameOrPath, int fallbackKeyCode) {
        namespace fs = std::filesystem;

        if (!nameOrPath.empty() && fs::exists(nameOrPath)) {
            UniqueFd fd(open(nameOrPath.data(), O_RDONLY | O_NONBLOCK));
            if (fd.valid()) {
                std::cout << "Using input device by path: " << getDeviceName(fd.get()) << " (" << nameOrPath << ")\n";
                return fs::path(nameOrPath);
            }
            std::cerr << "Warning: Cannot open specified input device path: " << nameOrPath << "\n";
        }

        const auto devices = listInputDevices();
        const std::string targetLower = toLower(nameOrPath);

        if (!nameOrPath.empty()) {
            for (const auto& dev : devices) {
                if (toLower(dev.name).find(targetLower) != std::string::npos) {
                    std::cout
                        << "Matched input device by name \""
                        << nameOrPath
                        << "\": "
                        << dev.name
                        << " ("
                        << dev.path.string()
                        << ")\n";
                    return dev.path;
                }
            }
            std::cerr
                << "Warning: No input device found matching name \""
                << nameOrPath
                << "\". Falling back to auto-detection.\n";
        }

        for (const auto& dev : devices) {
            UniqueFd fd(open(dev.path.c_str(), O_RDONLY | O_NONBLOCK));
            if (fd.valid() && hasKeyCode(fd.get(), fallbackKeyCode)) {
                std::cout << "Auto-detected input device: " << dev.name << " (" << dev.path.string() << ")\n";
                return dev.path;
            }
        }

        return std::nullopt;
    }

    std::optional<std::filesystem::path> findMouseDevice(std::string_view preferredNameOrPath) {
        return findDeviceByNameOrCapability(preferredNameOrPath, BTN_LEFT);
    }

    std::optional<std::filesystem::path> findKeyboardDevice(std::string_view preferredNameOrPath) {
        return findDeviceByNameOrCapability(preferredNameOrPath, KEY_A);
    }

} // namespace keywave
