#include "keywave/audio.hpp"
#include "test_framework.hpp"

extern bool TestAudioEngineLifecycle() {
  keywave::AudioEngine audio;
  TEST_ASSERT(!audio.IsReady());

  const bool inited = audio.Init();
  if (inited) {
    TEST_ASSERT(audio.IsReady());
    audio.SetVolume(0.5F);
    audio.SetVolume(1.0F);
    audio.SetChannelVolume(keywave::SoundChannel::kKeyboard, 0.8F);
    audio.SetChannelVolume(keywave::SoundChannel::kMouse, 0.4F);
    TEST_ASSERT(!audio.PreloadSound("non_existent_audio_file.wav"));
    TEST_ASSERT(!audio.PreloadSound(""));
    TEST_ASSERT(audio.PreloadSound("assets/sounds/mouses/mouse-click.mp3"));
    // Re-preloading already cached sound returns true immediately
    TEST_ASSERT(audio.PreloadSound("assets/sounds/mouses/mouse-click.mp3"));
    audio.PlaySound("assets/sounds/mouses/mouse-click.mp3", keywave::SoundChannel::kMouse);
    audio.PlaySound("non_existent_audio_file.wav", keywave::SoundChannel::kKeyboard);
    audio.PlaySound("non_existent_audio_file.wav", keywave::SoundChannel::kMouse);
    audio.Shutdown();
    TEST_ASSERT(!audio.IsReady());
  }
  return true;
}

extern void RunAudioTests() { RUN_TEST(TestAudioEngineLifecycle); }
