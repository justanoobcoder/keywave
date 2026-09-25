#include "test_framework.hpp"

void RunConfigTests();
void RunSoundpackTests();
void RunAudioTests();
void RunDeviceTests();

int main() {
  std::cout << "========================================\n";
  std::cout << "        Running Keywave Test Suite      \n";
  std::cout << "========================================\n";

  RunConfigTests();
  RunSoundpackTests();
  RunAudioTests();
  RunDeviceTests();

  std::cout << "========================================\n";
  std::cout << "Test Summary: " << g_passed_tests << "/" << g_total_tests
            << " passed";
  if (g_failed_tests > 0) {
    std::cout << " (" << g_failed_tests << " FAILED)\n";
    return 1;
  }
  std::cout << " \033[32m[ALL PASSED]\033[0m\n";
  return 0;
}
