#ifndef KEYWAVE_TEST_FRAMEWORK_HPP
#define KEYWAVE_TEST_FRAMEWORK_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline int g_total_tests = 0;
inline int g_passed_tests = 0;
inline int g_failed_tests = 0;

#define TEST_ASSERT(cond)                                               \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::cerr << "  \033[31m[FAILED]\033[0m Assertion failed: " #cond \
                << " (" << __FILE__ << ":" << __LINE__ << ")\n";        \
      return false;                                                     \
    }                                                                   \
  } while (0)

#define RUN_TEST(testFunc)                                          \
  do {                                                              \
    ++g_total_tests;                                                \
    std::cout << "[RUNNING] " << #testFunc << "... " << std::flush; \
    bool passed = false;                                            \
    {                                                               \
      SilenceOutput silencer;                                       \
      passed = testFunc();                                          \
    }                                                               \
    if (passed) {                                                   \
      ++g_passed_tests;                                             \
      std::cout << "\033[32m[PASS]\033[0m\n";                       \
    } else {                                                        \
      ++g_failed_tests;                                             \
      std::cout << "\033[31m[FAIL]\033[0m\n";                       \
    }                                                               \
  } while (0)

class SilenceOutput {
 public:
  SilenceOutput()
      : cout_buf_(std::cout.rdbuf(oss_cout_.rdbuf())),
        cerr_buf_(std::cerr.rdbuf(oss_cerr_.rdbuf())) {}

  ~SilenceOutput() {
    std::cout.rdbuf(cout_buf_);
    std::cerr.rdbuf(cerr_buf_);
  }

 private:
  std::ostringstream oss_cout_;
  std::ostringstream oss_cerr_;
  std::streambuf* cout_buf_;
  std::streambuf* cerr_buf_;
};

struct TempFile {
  std::filesystem::path path;
  explicit TempFile(const std::string& name, const std::string& content) {
    path = std::filesystem::temp_directory_path() / name;
    std::ofstream f(path);
    f << content;
  }
  ~TempFile() {
    std::error_code ec;
    std::filesystem::remove(path, ec);
  }
};

struct TempSoundpack {
  std::filesystem::path dir;
  explicit TempSoundpack(const std::string& name,
                         const std::string& jsonContent) {
    dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::create_directories(dir);
    std::ofstream f(dir / "config.json");
    f << jsonContent;
  }
  ~TempSoundpack() {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
};

#endif
