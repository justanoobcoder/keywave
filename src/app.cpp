#include "keywave/app.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>

#include "keywave/device.hpp"

namespace keywave {

namespace {

std::atomic<int> g_stop_fd{-1};

void SignalHandler(int) { App::Stop(); }

template <typename Callback>
void RunDeviceEventListener(const std::filesystem::path& device, Callback&& on_key_down) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
    return;
  }

  const int stop_fd = g_stop_fd.load(std::memory_order_relaxed);

  while (App::IsRunning()) {
    struct pollfd fds[2];
    nfds_t nfds = 1;
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    if (stop_fd >= 0) {
      fds[1].fd = stop_fd;
      fds[1].events = POLLIN;
      fds[1].revents = 0;
      nfds = 2;
    }

    const int timeout_ms = (stop_fd >= 0) ? -1 : 100;
    const int ret = poll(fds, nfds, timeout_ms);
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      }
      std::cerr << "poll error on " << device.string() << ": " << std::strerror(errno) << "\n";
      break;
    }

    if (!App::IsRunning()) {
      break;
    }

    if (stop_fd >= 0 && (fds[1].revents & POLLIN)) {
      break;
    }

    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      std::cerr << "Device disconnected: " << device.string() << "\n";
      break;
    }

    if (fds[0].revents & POLLIN) {
      struct input_event ev{};
      while (App::IsRunning()) {
        const ssize_t n = read(fd, &ev, sizeof(ev));
        if (std::cmp_equal(n, sizeof(ev))) {
          if (ev.type == EV_KEY && ev.value == 1) {
            on_key_down(ev.code);
          }
        } else {
          if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
          }
          if (n < 0 && errno == EINTR) {
            continue;
          }
          if (n < 0 && errno == ENODEV) {
            std::cerr << "Device removed: " << device.string() << "\n";
            close(fd);
            return;
          }
          break;
        }
      }
    }
  }

  close(fd);
}

}  // namespace

void App::Stop() {
  running_.store(false, std::memory_order_relaxed);
  const int sfd = g_stop_fd.load(std::memory_order_relaxed);
  if (sfd >= 0) {
    const uint64_t val = 1;
    [[maybe_unused]] auto res = write(sfd, &val, sizeof(val));
  }
}

bool App::IsRunning() noexcept { return running_.load(std::memory_order_relaxed); }

void App::PrintDeviceList() {
  const auto devices = device::ListInputDevices();
  if (devices.empty()) {
    std::cout << "No input devices accessible under /dev/input/.\n"
              << "Check that your user is in the 'input' group.\n";
    return;
  }

  std::cout << "Available input devices:\n";
  for (const auto& dev : devices) {
    std::string_view type = "Other";
    if (dev.is_keyboard && dev.is_mouse) {
      type = "Keyboard/Mouse";
    } else if (dev.is_keyboard) {
      type = "Keyboard";
    } else if (dev.is_mouse) {
      type = "Mouse";
    }

    std::cout << "  [" << std::left << std::setw(14) << type << "] \"" << dev.name << "\" ("
              << dev.path.string() << ")\n";
  }
}

void App::RunMouseListener(const std::filesystem::path& device,
                           const std::filesystem::path& sound_path, AudioEngine& audio) {
  const std::string sound_file = sound_path.string();
  RunDeviceEventListener(
    device, [&](uint16_t /*code*/) { audio.PlaySound(sound_file, SoundChannel::kMouse); });
}

void App::RunKeyboardListener(const std::filesystem::path& device, const SoundPack& soundpack,
                              AudioEngine& audio) {
  RunDeviceEventListener(device, [&](uint16_t code) {
    const auto sound_path = soundpack.SoundFor(code);
    if (sound_path) {
      audio.PlaySound(sound_path->string(), SoundChannel::kKeyboard);
    }
  });
}

bool App::ConfigureAudio(const Config& config, AudioEngine& audio) {
  if (!audio.Init()) {
    std::cerr << "Failed to initialize audio engine.\n";
    return false;
  }

  audio.SetVolume(config.GetVolume());
  audio.SetChannelVolume(SoundChannel::kKeyboard, config.GetKeyboardVolume());
  audio.SetChannelVolume(SoundChannel::kMouse, config.GetMouseVolume());

  return true;
}

void App::PrintBanner(const Config& config) {
  std::cout << "Keywave running with master volume: " << config.GetVolume();
  if (config.IsKeyboardVolumSet()) {
    std::cout << " (keyboard: " << config.GetKeyboardVolume() << ")";
  }
  if (config.IsMouseVolumSet()) {
    std::cout << " (mouse: " << config.GetMouseVolume() << ")";
  }
  std::cout << "\nPress Ctrl+C to quit.\n";
}

int App::Run(const Config& config) {
  AudioEngine audio;
  if (!ConfigureAudio(config, audio)) {
    return EXIT_FAILURE;
  }

  const auto soundpack = SoundPack::Load(config.GetKeyboardPack());
  if (!soundpack) {
    std::cerr << "Failed to load soundpack at " << config.GetKeyboardPack().string() << "\n";
    return EXIT_FAILURE;
  }

  const auto mouse_device = device::FindMouseDevice(config.GetMouseDevice());
  const auto keyboard_device = device::FindKeyboardDevice(config.GetKeyboardDevice());
  if (!mouse_device && !keyboard_device) {
    std::cerr << "No mouse or keyboard device found. Check that your user "
                 "is in the 'input' group.\n";
    return EXIT_FAILURE;
  }

  running_.store(true, std::memory_order_relaxed);

  const int stop_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  g_stop_fd.store(stop_fd, std::memory_order_relaxed);

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  std::thread mouse_thread;
  if (mouse_device) {
    std::cout << "Listening for clicks on " << mouse_device->string()
              << " (sound: " << config.GetMouseSound().string() << ")\n";
    mouse_thread =
      std::thread([&]() { RunMouseListener(*mouse_device, config.GetMouseSound(), audio); });
  }
  std::thread keyboard_thread;
  if (keyboard_device) {
    std::cout << "Listening for keystrokes on " << keyboard_device->string()
              << " (soundpack: " << soundpack->Name() << ")\n";
    keyboard_thread =
      std::thread([&]() { RunKeyboardListener(*keyboard_device, *soundpack, audio); });
  }

  PrintBanner(config);

  if (mouse_thread.joinable()) {
    mouse_thread.join();
  }
  if (keyboard_thread.joinable()) {
    keyboard_thread.join();
  }

  if (stop_fd >= 0) {
    close(stop_fd);
    g_stop_fd.store(-1, std::memory_order_relaxed);
  }

  return EXIT_SUCCESS;
}

}  // namespace keywave
