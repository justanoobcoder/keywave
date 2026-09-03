#include "keywave/device.h"

#include <array>
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

        [[nodiscard]] bool hasButtonLeft(int fd) {
            std::array<unsigned long, kKeyBitsWords> keybits{};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits.data()) < 0) {
                return false;
            }
            const std::size_t idx = BTN_LEFT / kBitsPerLong;
            const std::size_t bit = BTN_LEFT % kBitsPerLong;
            return (keybits[idx] >> bit) & 1UL;
        }

        [[nodiscard]] std::string deviceName(int fd) {
            char name[256] = "Unknown";
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            return name;
        }

    } // namespace

    std::optional<std::filesystem::path> findMouseDevice() {
        namespace fs = std::filesystem;

        std::error_code ec;
        if (!fs::exists("/dev/input", ec)) {
            std::cerr << "/dev/input does not exist on this system.\n";
            return std::nullopt;
        }

        for (const auto& entry : fs::directory_iterator("/dev/input", ec)) {
            const std::string filename = entry.path().filename().string();
            if (filename.rfind("event", 0) != 0)
                continue;

            UniqueFd fd(open(entry.path().c_str(), O_RDONLY | O_NONBLOCK));
            if (!fd.valid())
                continue;

            if (hasButtonLeft(fd.get())) {
                std::cout
                    << "Found mouse-like device: "
                    << deviceName(fd.get())
                    << " ("
                    << entry.path().string()
                    << ")\n";
                return entry.path();
            }
        }

        if (ec) {
            std::cerr << "Error scanning /dev/input: " << ec.message() << "\n";
        }
        return std::nullopt;
    }

} // namespace keywave
