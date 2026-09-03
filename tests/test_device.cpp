#include "test_framework.h"

#include "keywave/device.h"

bool test_deviceDetection_safeExecution() {
    const auto devices = keywave::listInputDevices();
    (void)devices;

    const auto mouse = keywave::findMouseDevice();
    const auto kb    = keywave::findKeyboardDevice();
    (void)mouse;
    (void)kb;

    const auto kbNonMatching = keywave::findKeyboardDevice("DefinitelyNotAnExistingKeyboardName123");
    (void)kbNonMatching;
    return true;
}

void runDeviceTests() {
    RUN_TEST(test_deviceDetection_safeExecution);
}
