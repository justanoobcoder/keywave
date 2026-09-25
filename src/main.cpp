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
#include <thread>

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
    std::string type;
    if (dev.isKeyboard && dev.isMouse) {
      type = "Keyboard/Mouse";
    } else if (dev.isKeyboard) {
      type = "Keyboard";
    } else if (dev.isMouse) {
      type = "Mouse";
    } else {
      type = "Other";
    }

    std::cout << "  [" << std::left << std::setw(14) << type << "] "
              << "\"" << dev.name << "\" (" << dev.path.string() << ")\n";
  }
}

void ListenAndPlayFixed(const std::filesystem::path& device,
                        const std::filesystem::path& soundPath,
                        keywave::AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": "
              << std::strerror(errno) << "\n";
    return;
  }

  const std::string sound_file = soundPath.string();
  struct input_event ev;
  while (g_running) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == static_cast<ssize_t>(sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        audio.PlaySound(sound_file, keywave::SoundChannel::kMouse);
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
}

void ListenAndPlayPerKey(const std::filesystem::path& device,
                         const keywave::SoundPack& soundpack,
                         keywave::AudioEngine& audio) {
  const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    std::cerr << "Failed to open " << device.string() << ": "
              << std::strerror(errno) << "\n";
    return;
  }

  struct input_event ev;
  while (g_running) {
    const ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == static_cast<ssize_t>(sizeof(ev))) {
      if (ev.type == EV_KEY && ev.value == 1) {
        const auto sound_path = soundpack.SoundFor(ev.code);
        if (sound_path) {
          audio.PlaySound(sound_path->string(),
                          keywave::SoundChannel::kKeyboard);
        }
      }
    } else {
      usleep(1000);
    }
  }
  close(fd);
}

}  // namespace

int main(int argc, char* argv[]) {
  const auto parse_result = keywave::ParseConfig(argc, argv);
  if (parse_result.status == keywave::ParseStatus::kHelpRequested ||
      parse_result.status == keywave::ParseStatus::kVersionRequested) {
    return 0;
  }
  if (parse_result.status == keywave::ParseStatus::kListDevicesRequested) {
    PrintDeviceList();
    return 0;
  }
  if (parse_result.status == keywave::ParseStatus::kError) {
    return 1;
  }

  const auto& config = parse_result.config;

  keywave::AudioEngine audio;
  if (!audio.Init()) {
    std::cerr << "Failed to initialize audio engine.\n";
    return 1;
  }
  audio.SetVolume(config.volume);
  if (config.keyboardVolume) {
    audio.SetChannelVolume(keywave::SoundChannel::kKeyboard,
                           *config.keyboardVolume);
  }
  if (config.mouseVolume) {
    audio.SetChannelVolume(keywave::SoundChannel::kMouse, *config.mouseVolume);
  }

  const auto soundpack = keywave::SoundPack::Load(config.keyboardPack);
  if (!soundpack) {
    std::cerr << "Failed to load soundpack at " << config.keyboardPack.string()
              << "\n";
    return 1;
  }

  const auto mouse = keywave::FindMouseDevice(config.mouseDevice);
  const auto keyboard = keywave::FindKeyboardDevice(config.keyboardDevice);

  if (!mouse && !keyboard) {
    std::cerr << "No mouse or keyboard device found. Check that your user "
                 "is in the 'input' group.\n";
    return 1;
  }

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  std::thread mouse_thread;
  std::thread keyboard_thread;
  if (mouse) {
    std::cout << "Listening for clicks on " << mouse->string()
              << " (sound: " << config.mouseSound.string() << ")\n";
    mouse_thread = std::thread(ListenAndPlayFixed, *mouse,
                               std::cref(config.mouseSound), std::ref(audio));
  }
  if (keyboard) {
    std::cout << "Listening for keystrokes on " << keyboard->string()
              << " (soundpack: " << soundpack->Name() << ")\n";
    keyboard_thread = std::thread(ListenAndPlayPerKey, *keyboard,
                                  std::cref(*soundpack), std::ref(audio));
  }

  std::cout << "Keywave running with master volume: " << config.volume;
  if (config.keyboardVolume) {
    std::cout << " (keyboard: " << *config.keyboardVolume << ")";
  }
  if (config.mouseVolume) {
    std::cout << " (mouse: " << *config.mouseVolume << ")";
  }
  std::cout << "\n";
  std::cout << "Press Ctrl+C to quit.\n";

  if (mouse_thread.joinable()) mouse_thread.join();
  if (keyboard_thread.joinable()) keyboard_thread.join();

  return 0;
}
