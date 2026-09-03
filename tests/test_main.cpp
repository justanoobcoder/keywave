#include "test_framework.h"

void runConfigTests();
void runSoundpackTests();
void runAudioTests();
void runDeviceTests();

int main() {
    std::cout << "========================================\n";
    std::cout << "        Running Keywave Test Suite      \n";
    std::cout << "========================================\n";

    runConfigTests();
    runSoundpackTests();
    runAudioTests();
    runDeviceTests();

    std::cout << "========================================\n";
    std::cout << "Test Summary: " << g_passedTests << "/" << g_totalTests << " passed";
    if (g_failedTests > 0) {
        std::cout << " (" << g_failedTests << " FAILED)\n";
        return 1;
    }
    std::cout << " \033[32m[ALL PASSED]\033[0m\n";
    return 0;
}
