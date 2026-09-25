#include "keywave/device.hpp"
#include "test_framework.hpp"

extern bool TestDeviceDetectionSafeExecution() {
  const auto devices = keywave::ListInputDevices();
  (void)devices;

  const auto mouse = keywave::FindMouseDevice();
  const auto kb = keywave::FindKeyboardDevice();
  (void)mouse;
  (void)kb;

  const auto kkb_non_matching =
    keywave::FindKeyboardDevice("DefinitelyNotAnExistingKeyboardName123");
  (void)kkb_non_matching;
  return true;
}

extern void RunDeviceTests() { RUN_TEST(TestDeviceDetectionSafeExecution); }
