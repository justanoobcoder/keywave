#include "keywave/audio.h"
#include "test_framework.h"

bool test_audioEngine_lifecycle() {
    keywave::AudioEngine audio;
    TEST_ASSERT(!audio.isReady());

    const bool inited = audio.init();
    if (inited) {
        TEST_ASSERT(audio.isReady());
        audio.setVolume(0.5F);
        audio.setVolume(1.0F);
        audio.setChannelVolume(keywave::SoundChannel::Keyboard, 0.8F);
        audio.setChannelVolume(keywave::SoundChannel::Mouse, 0.4F);
        audio.playSound("non_existent_audio_file.wav", keywave::SoundChannel::Keyboard);
        audio.playSound("non_existent_audio_file.wav", keywave::SoundChannel::Mouse);
        audio.shutdown();
        TEST_ASSERT(!audio.isReady());
    }
    return true;
}

void runAudioTests() { RUN_TEST(test_audioEngine_lifecycle); }
