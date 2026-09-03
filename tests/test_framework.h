#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline int g_totalTests  = 0;
inline int g_passedTests = 0;
inline int g_failedTests = 0;

#define TEST_ASSERT(cond)                                                                                              \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "  \033[31m[FAILED]\033[0m Assertion failed: " #cond << " (" << __FILE__ << ":" << __LINE__ \
                      << ")\n";                                                                                        \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#define RUN_TEST(testFunc)                                                                                             \
    do {                                                                                                               \
        ++g_totalTests;                                                                                                \
        std::cout << "[RUNNING] " << #testFunc << "... " << std::flush;                                               \
        bool passed = false;                                                                                           \
        {                                                                                                              \
            SilenceOutput silencer;                                                                                    \
            passed = testFunc();                                                                                       \
        }                                                                                                              \
        if (passed) {                                                                                                  \
            ++g_passedTests;                                                                                           \
            std::cout << "\033[32m[PASS]\033[0m\n";                                                                   \
        } else {                                                                                                       \
            ++g_failedTests;                                                                                           \
            std::cout << "\033[31m[FAIL]\033[0m\n";                                                                   \
        }                                                                                                              \
    } while (0)

class SilenceOutput {
  public:
    SilenceOutput()
        : m_coutBuf(std::cout.rdbuf(m_ossCout.rdbuf())), m_cerrBuf(std::cerr.rdbuf(m_ossCerr.rdbuf())) {}

    ~SilenceOutput() {
        std::cout.rdbuf(m_coutBuf);
        std::cerr.rdbuf(m_cerrBuf);
    }

  private:
    std::ostringstream m_ossCout;
    std::ostringstream m_ossCerr;
    std::streambuf*    m_coutBuf;
    std::streambuf*    m_cerrBuf;
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
    explicit TempSoundpack(const std::string& name, const std::string& jsonContent) {
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
