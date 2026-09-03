#include "keywave/audio.h"
#include "keywave/config.h"
#include "keywave/device.h"
#include "keywave/soundpack.h"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <linux/input.h>
#include <string>
#include <thread>
#include <unistd.h>

namespace {

    std::atomic<bool> g_running{true};
    void handleSignal(int) { g_running = false; }

    void printDeviceList() {
        const auto devices = keywave::listInputDevices();
        if (devices.empty()) {
            std::cout
                << "No input devices accessible under /dev/input/.\n"
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

            std::cout
                << "  ["
                << std::left
                << std::setw(14)
                << type
                << "] "
                << "\""
                << dev.name
                << "\" ("
                << dev.path.string()
                << ")\n";
        }
    }

    void listenAndPlayFixed(
        const std::filesystem::path& device, const std::filesystem::path& soundPath, keywave::AudioEngine& audio
    ) {
        const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
            return;
        }

        const std::string soundFile = soundPath.string();
        struct input_event ev;
        while (g_running) {
            const ssize_t n = read(fd, &ev, sizeof(ev));
            if (n == static_cast<ssize_t>(sizeof(ev))) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    audio.playSound(soundFile);
                }
            } else {
                usleep(1000);
            }
        }
        close(fd);
    }

    void listenAndPlayPerKey(
        const std::filesystem::path& device, const keywave::SoundPack& soundpack, keywave::AudioEngine& audio
    ) {
        const int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
            return;
        }

        struct input_event ev;
        while (g_running) {
            const ssize_t n = read(fd, &ev, sizeof(ev));
            if (n == static_cast<ssize_t>(sizeof(ev))) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    const auto soundPath = soundpack.soundFor(ev.code);
                    if (soundPath) {
                        audio.playSound(soundPath->string());
                    }
                }
            } else {
                usleep(1000);
            }
        }
        close(fd);
    }

} // namespace

int main(int argc, char* argv[]) {
    const auto parseResult = keywave::parseConfig(argc, argv);
    if (parseResult.status == keywave::ParseStatus::HelpRequested
        || parseResult.status == keywave::ParseStatus::VersionRequested) {
        return 0;
    }
    if (parseResult.status == keywave::ParseStatus::ListDevicesRequested) {
        printDeviceList();
        return 0;
    }
    if (parseResult.status == keywave::ParseStatus::Error) {
        return 1;
    }

    const auto& config = parseResult.config;

    keywave::AudioEngine audio;
    if (!audio.init()) {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }
    audio.setVolume(config.volume);

    const auto soundpack = keywave::SoundPack::load(config.keyboardPack);
    if (!soundpack) {
        std::cerr << "Failed to load soundpack at " << config.keyboardPack.string() << "\n";
        return 1;
    }

    const auto mouse = keywave::findMouseDevice(config.mouseDevice);
    const auto keyboard = keywave::findKeyboardDevice(config.keyboardDevice);

    if (!mouse && !keyboard) {
        std::cerr << "No mouse or keyboard device found. Check that your user "
                     "is in the 'input' group.\n";
        return 1;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::thread mouseThread, keyboardThread;
    if (mouse) {
        std::cout
            << "Listening for clicks on "
            << mouse->string()
            << " (sound: "
            << config.mouseSound.string()
            << ")\n";
        mouseThread = std::thread(listenAndPlayFixed, *mouse, std::cref(config.mouseSound), std::ref(audio));
    }
    if (keyboard) {
        std::cout
            << "Listening for keystrokes on "
            << keyboard->string()
            << " (soundpack: "
            << soundpack->name()
            << ")\n";
        keyboardThread = std::thread(listenAndPlayPerKey, *keyboard, std::cref(*soundpack), std::ref(audio));
    }

    std::cout << "Keywave running with volume: " << config.volume << "\n";
    std::cout << "Press Ctrl+C to quit.\n";

    if (mouseThread.joinable())
        mouseThread.join();
    if (keyboardThread.joinable())
        keyboardThread.join();

    return 0;
}
