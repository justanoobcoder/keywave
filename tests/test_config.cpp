#include "test_framework.h"

#include "keywave/config.h"

#include <string>
#include <vector>

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
        "keyboard_device = Custom Keyboard Device\n"
        "mouse_device = Custom Mouse Device\n"
    );

    const auto loaded = keywave::loadConfigFile(conf.path);
    TEST_ASSERT(loaded.has_value());
    TEST_ASSERT(loaded->volume >= 0.64F && loaded->volume <= 0.66F);
    TEST_ASSERT(loaded->keyboardPack == "/tmp/custom_keyboard");
    TEST_ASSERT(loaded->mouseSound == "/tmp/custom_mouse.mp3");
    TEST_ASSERT(loaded->keyboardDevice == "Custom Keyboard Device");
    TEST_ASSERT(loaded->mouseDevice == "Custom Mouse Device");
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
        "keyboard_dev = My USB Keyboard\n"
        "mouse_dev = My Wireless Mouse\n"
        "volume = 0.3\n"
    );

    const auto loaded = keywave::loadConfigFile(conf.path);
    TEST_ASSERT(loaded.has_value());
    TEST_ASSERT(loaded->volume >= 0.29F && loaded->volume <= 0.31F);
    TEST_ASSERT(loaded->keyboardPack == "/opt/soundpacks/keyboard");
    TEST_ASSERT(loaded->mouseSound == "/opt/sounds/mouse.wav");
    TEST_ASSERT(loaded->keyboardDevice == "My USB Keyboard");
    TEST_ASSERT(loaded->mouseDevice == "My Wireless Mouse");
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
        "keyboard_device = Old Keyboard\n"
        "mouse_device = Old Mouse\n"
    );

    std::string confStr = conf.path.string();
    std::vector<std::string> args = {"keywave",       "-c", confStr,          "-v", "0.9",          "-k",
                                     "/cli/keyboard", "-m", "/cli/mouse.wav", "-K", "New Keyboard", "-M",
                                     "New Mouse"};

    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::Success);
    TEST_ASSERT(res.config.volume >= 0.89F && res.config.volume <= 0.91F);
    TEST_ASSERT(res.config.keyboardPack == "/cli/keyboard");
    TEST_ASSERT(res.config.mouseSound == "/cli/mouse.wav");
    TEST_ASSERT(res.config.keyboardDevice == "New Keyboard");
    TEST_ASSERT(res.config.mouseDevice == "New Mouse");
    return true;
}

bool test_parseConfig_helpOption() {
    std::vector<std::string> args = {"keywave", "--help"};
    std::vector<char*>       argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::HelpRequested);
    return true;
}

bool test_parseConfig_listDevicesOption() {
    std::vector<std::string> args = {"keywave", "--list-devices"};
    std::vector<char*>       argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::ListDevicesRequested);
    return true;
}

bool test_parseConfig_invalidVolumePercent() {
    std::vector<std::string> args = {"keywave", "-v", "80%"};
    std::vector<char*>       argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::Error);
    return true;
}

bool test_parseConfig_invalidVolumeNegative() {
    std::vector<std::string> args = {"keywave", "-v", "-0.5"};
    std::vector<char*>       argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::Error);
    return true;
}

bool test_parseConfig_nonExistentConfigFlag() {
    std::vector<std::string> args = {"keywave", "-c", "/path/to/missing_config_test.conf"};
    std::vector<char*>       argv;
    argv.reserve(args.size());
    for (auto& s : args)
        argv.push_back(s.data());

    const auto res = keywave::parseConfig(static_cast<int>(argv.size()), argv.data());
    TEST_ASSERT(res.status == keywave::ParseStatus::Error);
    return true;
}

void runConfigTests() {
    RUN_TEST(test_defaultConfigPath);
    RUN_TEST(test_loadConfigFile_valid);
    RUN_TEST(test_loadConfigFile_aliases_and_comments);
    RUN_TEST(test_loadConfigFile_nonExistent);
    RUN_TEST(test_parseConfig_cliOverrides);
    RUN_TEST(test_parseConfig_helpOption);
    RUN_TEST(test_parseConfig_listDevicesOption);
    RUN_TEST(test_parseConfig_invalidVolumePercent);
    RUN_TEST(test_parseConfig_invalidVolumeNegative);
    RUN_TEST(test_parseConfig_nonExistentConfigFlag);
}
