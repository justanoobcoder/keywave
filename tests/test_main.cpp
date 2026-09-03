#include "keywave/audio.h"
#include "keywave/config.h"
#include "keywave/device.h"
#include "keywave/soundpack.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

    int g_totalTests = 0;
    int g_passedTests = 0;
    int g_failedTests = 0;

#define TEST_ASSERT(cond)                                                                                              \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr                                                                                                  \
                << "  \033[31m[FAILED]\033[0m Assertion failed: " #cond                                                \
                << " ("                                                                                                \
                << __FILE__                                                                                            \
                << ":"                                                                                                 \
                << __LINE__                                                                                            \
                << ")\n";                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

#define RUN_TEST(testFunc)                                                                                             \
    do {                                                                                                               \
        ++g_totalTests;                                                                                                \
        std::cout << "[RUNNING] " << #testFunc << "... " << std::flush;                                                \
        bool passed = false;                                                                                           \
        {                                                                                                              \
            SilenceOutput silencer;                                                                                    \
            passed = testFunc();                                                                                       \
        }                                                                                                              \
        if (passed) {                                                                                                  \
            ++g_passedTests;                                                                                           \
            std::cout << "\033[32m[PASS]\033[0m\n";                                                                    \
        } else {                                                                                                       \
            ++g_failedTests;                                                                                           \
            std::cout << "\033[31m[FAIL]\033[0m\n";                                                                    \
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
        std::streambuf* m_coutBuf;
        std::streambuf* m_cerrBuf;
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

    bool test_defaultConfigPath() {
        const auto path = keywave::getDefaultConfigPath();
        TEST_ASSERT(!path.empty());
        TEST_ASSERT(path.filename() == "keywave.conf");
        return true;
    }

    bool test_loadConfigFile_valid() {
        TempFile conf(
            "keywave_test_valid.conf",
            "# Test comment\n"
            "volume = 0.65\n"
            "keyboard_pack = /tmp/custom_keyboard\n"
            "mouse_sound = /tmp/custom_mouse.mp3\n"
        );

        const auto loaded = keywave::loadConfigFile(conf.path);
        TEST_ASSERT(loaded.has_value());
        TEST_ASSERT(loaded->volume >= 0.64F && loaded->volume <= 0.66F);
        TEST_ASSERT(loaded->keyboardPack == "/tmp/custom_keyboard");
        TEST_ASSERT(loaded->mouseSound == "/tmp/custom_mouse.mp3");
        TEST_ASSERT(loaded->configPath == conf.path);
        return true;
    }

    bool test_loadConfigFile_aliases_and_comments() {
        TempFile conf(
            "keywave_test_aliases.conf",
            "; Semicolon comment\n"
            "[section_header]\n"
            "keyboard_soundpack = /opt/soundpacks/keyboard\n"
            "mouse = /opt/sounds/mouse.wav\n"
            "volume = 0.3\n"
        );

        const auto loaded = keywave::loadConfigFile(conf.path);
        TEST_ASSERT(loaded.has_value());
        TEST_ASSERT(loaded->volume >= 0.29F && loaded->volume <= 0.31F);
        TEST_ASSERT(loaded->keyboardPack == "/opt/soundpacks/keyboard");
        TEST_ASSERT(loaded->mouseSound == "/opt/sounds/mouse.wav");
        return true;
    }

    bool test_loadConfigFile_nonExistent() {
        const auto loaded = keywave::loadConfigFile("/path/to/definitely/non_existent_file.conf");
        TEST_ASSERT(!loaded.has_value());
        return true;
    }

    bool test_parseConfig_cliOverrides() {
        TempFile conf(
            "keywave_test_override.conf",
            "volume = 0.5\n"
            "keyboard_pack = /original/keyboard\n"
            "mouse_sound = /original/mouse.mp3\n"
        );

        std::string confStr = conf.path.string();
        std::vector<std::string> args = {"keywave",       "-c", confStr,         "-v", "0.9", "-k",
                                         "/cli/keyboard", "-m", "/cli/mouse.wav"};

        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args)
            argv.push_back(s.data());

        const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
        TEST_ASSERT(res.status == keywave::ParseStatus::Success);
        TEST_ASSERT(res.config.volume >= 0.89F && res.config.volume <= 0.91F);
        TEST_ASSERT(res.config.keyboardPack == "/cli/keyboard");
        TEST_ASSERT(res.config.mouseSound == "/cli/mouse.wav");
        return true;
    }

    bool test_parseConfig_helpOption() {
        std::vector<std::string> args = {"keywave", "--help"};
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args)
            argv.push_back(s.data());

        const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
        TEST_ASSERT(res.status == keywave::ParseStatus::HelpRequested);
        return true;
    }

    bool test_parseConfig_invalidVolumePercent() {
        std::vector<std::string> args = {"keywave", "-v", "80%"};
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args)
            argv.push_back(s.data());

        const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
        TEST_ASSERT(res.status == keywave::ParseStatus::Error);
        return true;
    }

    bool test_parseConfig_invalidVolumeNegative() {
        std::vector<std::string> args = {"keywave", "-v", "-0.5"};
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args)
            argv.push_back(s.data());

        const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
        TEST_ASSERT(res.status == keywave::ParseStatus::Error);
        return true;
    }

    bool test_parseConfig_nonExistentConfigFlag() {
        std::vector<std::string> args = {"keywave", "-c", "/path/to/missing_config_test.conf"};
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args)
            argv.push_back(s.data());

        const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
        TEST_ASSERT(res.status == keywave::ParseStatus::Error);
        return true;
    }

    bool test_soundpack_loadValid() {
        TempSoundpack pack(
            "soundpack_test_valid",
            R"({
                           "name": "Test Soundpack",
                           "sound": "sound.wav",
                           "defines": {
                               "30": "a.wav",
                               "31": "s.wav"
                           }
                       })"
        );

        const auto soundpack = keywave::SoundPack::load(pack.dir);
        TEST_ASSERT(soundpack.has_value());
        TEST_ASSERT(soundpack->name() == "Test Soundpack");
        TEST_ASSERT(soundpack->mappedKeyCount() == 2);

        const auto soundA = soundpack->soundFor(30);
        TEST_ASSERT(soundA.has_value());
        TEST_ASSERT(soundA->filename() == "a.wav");

        const auto soundS = soundpack->soundFor(31);
        TEST_ASSERT(soundS.has_value());
        TEST_ASSERT(soundS->filename() == "s.wav");

        const auto soundUnmapped = soundpack->soundFor(999);
        TEST_ASSERT(!soundUnmapped.has_value());
        return true;
    }

    bool test_soundpack_loadMissingConfig() {
        const auto soundpack = keywave::SoundPack::load("/tmp/non_existent_soundpack_dir");
        TEST_ASSERT(!soundpack.has_value());
        return true;
    }

    bool test_soundpack_loadMalformedJson() {
        TempSoundpack pack("soundpack_test_malformed", "{ invalid json content");
        const auto soundpack = keywave::SoundPack::load(pack.dir);
        TEST_ASSERT(!soundpack.has_value());
        return true;
    }

    bool test_soundpack_loadMissingDefines() {
        TempSoundpack pack("soundpack_test_nodefines", R"({"name": "No defines pack"})");
        const auto soundpack = keywave::SoundPack::load(pack.dir);
        TEST_ASSERT(!soundpack.has_value());
        return true;
    }

    bool test_audioEngine_lifecycle() {
        keywave::AudioEngine audio;
        TEST_ASSERT(!audio.isReady());

        const bool inited = audio.init();
        if (inited) {
            TEST_ASSERT(audio.isReady());
            audio.setVolume(0.5F);
            audio.setVolume(1.0F);
            audio.playSound("non_existent_audio_file.wav");
            audio.shutdown();
            TEST_ASSERT(!audio.isReady());
        }
        return true;
    }

    bool test_deviceDetection_safeExecution() {
        const auto mouse = keywave::findMouseDevice();
        const auto kb = keywave::findKeyboardDevice();
        (void)mouse;
        (void)kb;
        return true;
    }

} // namespace

int main() {
    std::cout << "========================================\n";
    std::cout << "        Running Keywave Test Suite      \n";
    std::cout << "========================================\n";

    RUN_TEST(test_defaultConfigPath);
    RUN_TEST(test_loadConfigFile_valid);
    RUN_TEST(test_loadConfigFile_aliases_and_comments);
    RUN_TEST(test_loadConfigFile_nonExistent);
    RUN_TEST(test_parseConfig_cliOverrides);
    RUN_TEST(test_parseConfig_helpOption);
    RUN_TEST(test_parseConfig_invalidVolumePercent);
    RUN_TEST(test_parseConfig_invalidVolumeNegative);
    RUN_TEST(test_parseConfig_nonExistentConfigFlag);

    RUN_TEST(test_soundpack_loadValid);
    RUN_TEST(test_soundpack_loadMissingConfig);
    RUN_TEST(test_soundpack_loadMalformedJson);
    RUN_TEST(test_soundpack_loadMissingDefines);

    RUN_TEST(test_audioEngine_lifecycle);

    RUN_TEST(test_deviceDetection_safeExecution);

    std::cout << "========================================\n";
    std::cout << "Test Summary: " << g_passedTests << "/" << g_totalTests << " passed";
    if (g_failedTests > 0) {
        std::cout << " (" << g_failedTests << " FAILED)\n";
        return 1;
    }
    std::cout << " \033[32m[ALL PASSED]\033[0m\n";
    return 0;
}
