#include "test_framework.h"

#include "keywave/audio.h"

bool test_audioEngine_lifecycle() {
    keywave::AudioEngine audio;
    TEST_ASSERT(!audio.isReady());

    const bool inited = audio.init();
    if (inited) {
        TEST_ASSERT(audio.isReady());
        audio.setVolume(0.5F);
        audio.setVolume(1.0F);
        audio.playSound("non_existent_audio_file.wav");
        audio.shutdown();
        TEST_ASSERT(!audio.isReady());
    }
    return true;
}

void runAudioTests() {
    RUN_TEST(test_audioEngine_lifecycle);
}
