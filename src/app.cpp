#include "keywave/app.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>

#include "keywave/device.hpp"

namespace keywave {

void App::PrintDeviceList() {
  const auto devices = ListInputDevices();
  if (devices.empty()) {
    std::cout << "No input devices accessible under /dev/input/.\n"
              << "Check that your user is in the 'input' group.\n";
    return;
  }

  std::cout << "Available input devices:\n";
  for (const auto& dev : devices) {
    std::string_view type = "Other";
    if (dev.isKeyboard && dev.isMouse) {
      type = "Keyboard/Mouse";
    } else if (dev.isKeyboard) {
      type = "Keyboard";
    } else if (dev.isMouse) {
      type = "Mouse";
    }

    std::cout << "  [" << std::left << std::setw(14) << type << "] \"" << dev.name << "\" ("
              << dev.path.string() << ")\n";
  }
}

void App::RunMouseListener(const std::filesystem::path& device,
                           const std::filesystem::path& sound_path, AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
    return;
  }

  const std::string sound_file = sound_path.string();
  struct input_event ev;
  while (running_) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (std::cmp_equal(n, sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        audio.PlaySound(sound_file, SoundChannel::kMouse);
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
}

void App::RunKeyboardListener(const std::filesystem::path& device, const SoundPack& soundpack,
                              AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
    return;
  }

  struct input_event ev;
  while (running_) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (std::cmp_equal(n, sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        const auto sound_path = soundpack.SoundFor(ev.code);
        if (sound_path) {
          audio.PlaySound(sound_path->string(), SoundChannel::kKeyboard);
        }
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
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

  const auto mouse_device = FindMouseDevice(config.GetMouseDevice());
  const auto keyboard_device = FindKeyboardDevice(config.GetKeyboardDevice());
  if (!mouse_device && !keyboard_device) {
    std::cerr << "No mouse or keyboard device found. Check that your user "
                 "is in the 'input' group.\n";
    return EXIT_FAILURE;
  }

  std::signal(SIGINT, [](int) { running_ = false; });
  std::signal(SIGTERM, [](int) { running_ = false; });

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

  return EXIT_SUCCESS;
}

}  // namespace keywave
