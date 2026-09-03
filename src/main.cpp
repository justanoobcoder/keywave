#include "keywave/audio.h"
#include "keywave/device.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <unistd.h>

namespace {
    volatile sig_atomic_t g_running = 1;
    void handleSignal(int) { g_running = 0; }
} // namespace

int main() {
    keywave::AudioEngine audio;
    if (!audio.init()) {
        std::cerr << "Failed to initialize audio engine.\n";
        return 1;
    }
    audio.setVolume(1.0F);

    auto device = keywave::findMouseDevice();
    if (!device) {
        std::cerr << "No mouse device found. Check that your user is in the "
                     "'input' group.\n";
        return 1;
    }

    int fd = open(device->c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cerr << "Failed to open " << device->string() << ": " << std::strerror(errno) << "\n";
        return 1;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::cout << "Listening on " << device->string() << ". Press Ctrl+C to quit.\n";

    struct input_event ev;
    while (g_running) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == static_cast<ssize_t>(sizeof(ev))) {
            if (ev.type == EV_KEY && ev.value == 1) {
                audio.playSound("assets/sounds/mouses/mouse-click.mp3");
            }
        } else {
            usleep(1000);
        }
    }

    close(fd);
    return 0;
}
