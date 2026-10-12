#include <string>
#include <vector>

#include "keywave/config.hpp"
#include "test_framework.hpp"

extern bool TestDefaultConfigPath() {
  keywave::Config config;
  const auto path = config.GetDefaultConfigPath();
  TEST_ASSERT(!path.empty());
  TEST_ASSERT(path.filename() == "keywave.conf");
  return true;
}

extern bool TestLoadConfigFileValid() {
  keywave::Config config;
  TempFile conf("keywave_test_valid.conf",
                "# Test comment\n"
                "volume = 0.65\n"
                "keyboard_volume = 0.8\n"
                "mouse_volume = 0.4\n"
                "keyboard_pack = /tmp/custom_keyboard\n"
                "mouse_sound = /tmp/custom_mouse.mp3\n"
                "keyboard_device = Custom Keyboard Device\n"
                "mouse_device = Custom Mouse Device\n");

  const auto loaded = config.LoadConfigFile(conf.path);
  TEST_ASSERT(loaded.has_value());
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(loaded->GetVolume() >= 0.64F && loaded->GetVolume() <= 0.66F);
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(loaded->IsKeyboardVolumSet() && loaded->GetKeyboardVolume() >= 0.79F &&
              loaded->GetKeyboardVolume() <= 0.81F);
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(loaded->IsMouseVolumSet() && loaded->GetMouseVolume() >= 0.39F &&
              loaded->GetMouseVolume() <= 0.41F);
  TEST_ASSERT(loaded->GetKeyboardPack() == "/tmp/custom_keyboard");
  TEST_ASSERT(loaded->GetMouseSound() == "/tmp/custom_mouse.mp3");
  TEST_ASSERT(loaded->GetKeyboardDevice() == "Custom Keyboard Device");
  TEST_ASSERT(loaded->GetMouseDevice() == "Custom Mouse Device");
  TEST_ASSERT(loaded->GetConfigPath() == conf.path);
  return true;
}

extern bool TestLoadConfigFileAliasesAndComments() {
  keywave::Config config;
  TempFile conf("keywave_test_aliases.conf",
                "; Semicolon comment\n"
                "[section_header]\n"
                "keyboard_soundpack = /opt/soundpacks/keyboard\n"
                "mouse = /opt/sounds/mouse.wav\n"
                "keyboard_dev = My USB Keyboard\n"
                "mouse_dev = My Wireless Mouse\n"
                "volume = 0.3\n");

  const auto loaded = config.LoadConfigFile(conf.path);
  TEST_ASSERT(loaded.has_value());
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(loaded->GetVolume() >= 0.29F && loaded->GetVolume() <= 0.31F);
  TEST_ASSERT(loaded->GetKeyboardPack() == "/opt/soundpacks/keyboard");
  TEST_ASSERT(loaded->GetMouseSound() == "/opt/sounds/mouse.wav");
  TEST_ASSERT(loaded->GetKeyboardDevice() == "My USB Keyboard");
  TEST_ASSERT(loaded->GetMouseDevice() == "My Wireless Mouse");
  return true;
}

extern bool TestLoadConfigFileNonExistent() {
  keywave::Config config;
  const auto loaded = config.LoadConfigFile("/path/to/definitely/non_existent_file.conf");
  TEST_ASSERT(!loaded.has_value());
  return true;
}

extern bool TestParseConfigCliOverrides() {
  TempFile conf("keywave_test_override.conf",
                "volume = 0.5\n"
                "keyboard_pack = /original/keyboard\n"
                "mouse_sound = /original/mouse.mp3\n"
                "keyboard_device = Old Keyboard\n"
                "mouse_device = Old Mouse\n");

  std::string conf_str = conf.path.string();
  std::vector<std::string> args = {
    "keywave",        "-c",          conf_str,       "-v", "0.9",           "--keyboard-vol",
    "0.75",           "--mouse-vol", "0.35",         "-k", "/cli/keyboard", "-m",
    "/cli/mouse.wav", "-K",          "New Keyboard", "-M", "New Mouse"};

  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kSuccess);
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(res.config.GetVolume() >= 0.89F && res.config.GetVolume() <= 0.91F);
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(res.config.IsKeyboardVolumSet() && res.config.GetKeyboardVolume() >= 0.74F &&
              res.config.GetKeyboardVolume() <= 0.76F);
  // NOLINTNEXTLINE(readability-simplify-boolean-expr)
  TEST_ASSERT(res.config.IsMouseVolumSet() && res.config.GetMouseVolume() >= 0.34F &&
              res.config.GetMouseVolume() <= 0.36F);
  TEST_ASSERT(res.config.GetKeyboardPack() == "/cli/keyboard");
  TEST_ASSERT(res.config.GetMouseSound() == "/cli/mouse.wav");
  TEST_ASSERT(res.config.GetKeyboardDevice() == "New Keyboard");
  TEST_ASSERT(res.config.GetMouseDevice() == "New Mouse");
  return true;
}

extern bool TestParseConfigHelpOption() {
  std::vector<std::string> args = {"keywave", "--help"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kHelpRequested);
  return true;
}

extern bool TestParseConfigListDevicesOption() {
  std::vector<std::string> args = {"keywave", "--list-devices"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kListDevicesRequested);
  return true;
}

extern bool TestParseConfigInvalidVolumePercent() {
  std::vector<std::string> args = {"keywave", "-v", "80%"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kError);
  return true;
}

extern bool TestParseConfigInvalidVolumeNegative() {
  std::vector<std::string> args = {"keywave", "-v", "-0.5"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kError);
  return true;
}

extern bool TestParseConfigNonExistentConfigFlag() {
  std::vector<std::string> args = {"keywave", "-c", "/path/to/missing_config_test.conf"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kError);
  return true;
}

extern bool TestParseConfigVersionOption() {
  std::vector<std::string> args = {"keywave", "--version"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kVersionRequested);
  return true;
}

extern bool TestParseConfigVersionShortOption() {
  std::vector<std::string> args = {"keywave", "-V"};
  std::vector<char*> argv;
  argv.reserve(args.size());
  for (auto& s : args) argv.push_back(s.data());

  const auto res = keywave::ParseConfig(static_cast<int>(argv.size()), argv.data());
  TEST_ASSERT(res.status == keywave::ParseStatus::kVersionRequested);
  return true;
}

extern void RunConfigTests() {
  RUN_TEST(TestDefaultConfigPath);
  RUN_TEST(TestLoadConfigFileValid);
  RUN_TEST(TestLoadConfigFileAliasesAndComments);
  RUN_TEST(TestLoadConfigFileNonExistent);
  RUN_TEST(TestParseConfigCliOverrides);
  RUN_TEST(TestParseConfigHelpOption);
  RUN_TEST(TestParseConfigListDevicesOption);
  RUN_TEST(TestParseConfigVersionOption);
  RUN_TEST(TestParseConfigVersionShortOption);
  RUN_TEST(TestParseConfigInvalidVolumePercent);
  RUN_TEST(TestParseConfigInvalidVolumeNegative);
  RUN_TEST(TestParseConfigNonExistentConfigFlag);
}
