#include "keywave/audio.h"

#include <cstdio>
#include <iostream>

int main() {
    keywave::AudioEngine audio;
    if (audio.init()) {
        audio.setVolume(1.0F);
        audio.playSound("assets/sounds/mouses/mouse-click.mp3");
    }

    std::cout << "Press any key to exit!";

    getchar();

    return 0;
}
