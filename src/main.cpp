#include "keywave/audio.h"
#include "keywave/device.h"
#include "keywave/soundpack.h"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <linux/input.h>
#include <string>
#include <thread>
#include <unistd.h>

namespace {

    std::atomic<bool> g_running{true};
    void handleSignal(int) { g_running = false; }

    // Listens on `device` and plays a fixed sound on every button/key press.
    // Used for the mouse thread, where all clicks share one sound.
    void listenAndPlayFixed(const std::filesystem::path& device, std::string soundFile, keywave::AudioEngine& audio) {
        int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
            return;
        }

        struct input_event ev;
        while (g_running) {
            ssize_t n = read(fd, &ev, sizeof(ev));
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

    // Listens on `device` and plays a sound per key defined by the soundpack.
    void listenAndPlayPerKey(
        const std::filesystem::path& device, const keywave::SoundPack& soundpack, keywave::AudioEngine& audio
    ) {
        int fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open " << device.string() << ": " << std::strerror(errno) << "\n";
            return;
        }

        struct input_event ev;
        while (g_running) {
            ssize_t n = read(fd, &ev, sizeof(ev));
            if (n == static_cast<ssize_t>(sizeof(ev))) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    auto soundPath = soundpack.soundFor(ev.code);
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

int main() {
    keywave::AudioEngine audio;
    if (!audio.init()) {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }
    audio.setVolume(1.0F);

    const std::filesystem::path soundpackDir = "assets/sounds/keyboards/cherrymx-red-abs";
    auto soundpack = keywave::SoundPack::load(soundpackDir);
    if (!soundpack) {
        std::cerr << "Failed to load soundpack at " << soundpackDir.string() << "\n";
        return 1;
    }

    auto mouse = keywave::findMouseDevice();
    auto keyboard = keywave::findKeyboardDevice();

    if (!mouse && !keyboard) {
        std::cerr << "No mouse or keyboard device found. Check that your user "
                     "is in the 'input' group.\n";
        return 1;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::thread mouseThread, keyboardThread;
    if (mouse) {
        std::cout << "Listening for clicks on " << mouse->string() << "\n";
        mouseThread =
            std::thread(listenAndPlayFixed, *mouse, "./assets/sounds/mouses/mouse-click.mp3", std::ref(audio));
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

    std::cout << "Press Ctrl+C to quit.\n";

    if (mouseThread.joinable())
        mouseThread.join();
    if (keyboardThread.joinable())
        keyboardThread.join();

    return 0;
}
