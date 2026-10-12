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

extern void RunDeviceTests() { RUN_TEST(TestDeviceDetectionSafeExecution); }
