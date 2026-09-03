#include "keywave/audio.h"
#include "keywave/device.h"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <linux/input.h>
#include <string_view>
#include <thread>
#include <unistd.h>

namespace {

    std::atomic<bool> g_running{true};
    void handleSignal(int) { g_running = false; }

    // Opens `device` and calls audio.playSound(soundFile) on every key/button
    // press (EV_KEY, value == 1) until g_running becomes false.
    void listenAndPlay(const std::filesystem::path& device, std::string_view soundFile, keywave::AudioEngine& audio) {
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

} // namespace

int main() {
    keywave::AudioEngine audio;
    if (!audio.init()) {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }
    audio.setVolume(1.0F);

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
        mouseThread = std::thread(listenAndPlay, *mouse, "assets/sounds/mouses/mouse-click.mp3", std::ref(audio));
    }
    if (keyboard) {
        std::cout << "Listening for keystrokes on " << keyboard->string() << "\n";
        keyboardThread = std::thread(listenAndPlay, *keyboard, "sound.wav", std::ref(audio));
    }

    std::cout << "Press Ctrl+C to quit.\n";

    if (mouseThread.joinable())
        mouseThread.join();
    if (keyboardThread.joinable())
        keyboardThread.join();

    return 0;
}
