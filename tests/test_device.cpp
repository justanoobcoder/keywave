#include "keywave/app.hpp"
#include "keywave/device.hpp"
#include "test_framework.hpp"

extern bool TestDeviceDetectionSafeExecution() {
  const auto devices = keywave::device::ListInputDevices();
  (void)devices;

  const auto mouse = keywave::device::FindMouseDevice();
  const auto kb = keywave::device::FindKeyboardDevice();
  (void)mouse;
  (void)kb;

  const auto kkb_non_matching =
    keywave::device::FindKeyboardDevice("DefinitelyNotAnExistingKeyboardName123");
  (void)kkb_non_matching;
  return true;
}

extern bool TestAppLifecycle() {
  TEST_ASSERT(keywave::App::IsRunning());
  keywave::App::Stop();
  TEST_ASSERT(!keywave::App::IsRunning());
  return true;
}

extern bool TestAppNonExistentDevice() {
  keywave::App app;
  keywave::AudioEngine audio;
  keywave::SoundPack pack;
  // Opening non-existent device should fail gracefully without crashing
  app.RunKeyboardListener("/dev/input/non_existent_event_999", pack, audio);
  app.RunMouseListener("/dev/input/non_existent_event_999", "/non/existent/click.wav", audio);
  return true;
}

extern void RunDeviceTests() {
  RUN_TEST(TestDeviceDetectionSafeExecution);
  RUN_TEST(TestAppLifecycle);
  RUN_TEST(TestAppNonExistentDevice);
}
