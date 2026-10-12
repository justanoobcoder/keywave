#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "keywave/audio.hpp"
#include "keywave/config.hpp"
#include "keywave/device.hpp"
#include "keywave/soundpack.hpp"

namespace {

std::atomic<bool> g_running{true};

void HandleSignal(int) { g_running = false; }

void PrintDeviceList() {
  const auto devices = keywave::ListInputDevices();
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

void RunMouseListener(const std::filesystem::path& device, const std::filesystem::path& sound_path,
                      keywave::AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
    return;
  }

  const std::string sound_file = sound_path.string();
  struct input_event ev;
  while (g_running) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (std::cmp_equal(n, sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        audio.PlaySound(sound_file, keywave::SoundChannel::kMouse);
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
}

void RunKeyboardListener(const std::filesystem::path& device, const keywave::SoundPack& soundpack,
                         keywave::AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
    return;
  }

  struct input_event ev;
  while (g_running) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (std::cmp_equal(n, sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        const auto sound_path = soundpack.SoundFor(ev.code);
        if (sound_path) {
          audio.PlaySound(sound_path->string(), keywave::SoundChannel::kKeyboard);
        }
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
}

bool ConfigureAudio(const keywave::Config& config, keywave::AudioEngine& audio) {
  if (!audio.Init()) {
    std::cerr << "Failed to initialize audio engine.\n";
    return false;
  }
  audio.SetVolume(config.volume);
  if (config.keyboardVolume) {
    audio.SetChannelVolume(keywave::SoundChannel::kKeyboard, *config.keyboardVolume);
  }
  if (config.mouseVolume) {
    audio.SetChannelVolume(keywave::SoundChannel::kMouse, *config.mouseVolume);
  }
  return true;
}

void PrintBanner(const keywave::Config& config) {
  std::cout << "Keywave running with master volume: " << config.volume;
  if (config.keyboardVolume) {
    std::cout << " (keyboard: " << *config.keyboardVolume << ")";
  }
  if (config.mouseVolume) {
    std::cout << " (mouse: " << *config.mouseVolume << ")";
  }
  std::cout << "\nPress Ctrl+C to quit.\n";
}

int RunKeywave(const keywave::Config& config) {
  keywave::AudioEngine audio;
  if (!ConfigureAudio(config, audio)) {
    return 1;
  }

  const auto soundpack = keywave::SoundPack::Load(config.keyboardPack);
  if (!soundpack) {
    std::cerr << "Failed to load soundpack at " << config.keyboardPack.string() << "\n";
    return 1;
  }

  const auto mouse_device = keywave::FindMouseDevice(config.mouseDevice);
  const auto keyboard_device = keywave::FindKeyboardDevice(config.keyboardDevice);
  if (!mouse_device && !keyboard_device) {
    std::cerr << "No mouse or keyboard device found. Check that your user "
                 "is in the 'input' group.\n";
    return 1;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  std::thread mouse_thread;
  if (mouse_device) {
    std::cout << "Listening for clicks on " << mouse_device->string()
              << " (sound: " << config.mouseSound.string() << ")\n";
    mouse_thread =
      std::thread([&]() { RunMouseListener(*mouse_device, config.mouseSound, audio); });
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

  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  const auto parse_result = keywave::ParseConfig(argc, argv);
  switch (parse_result.status) {
    case keywave::ParseStatus::kHelpRequested:
    case keywave::ParseStatus::kVersionRequested:
      return 0;
    case keywave::ParseStatus::kListDevicesRequested:
      PrintDeviceList();
      return 0;
    case keywave::ParseStatus::kError:
      return 1;
    case keywave::ParseStatus::kSuccess:
      return RunKeywave(parse_result.config);
  }
  return 1;
}
