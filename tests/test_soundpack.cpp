#include "keywave/soundpack.hpp"
#include "test_framework.hpp"

extern bool TestSoundpackLoadValid() {
  TempSoundpack pack("soundpack_test_valid",
                     R"({
               "name": "Test Soundpack",
               "sound": "sound.wav",
               "defines": {
                   "30": "a.wav",
                   "31": "s.wav"
               }
           })");

  const auto soundpack = keywave::SoundPack::Load(pack.dir);
  TEST_ASSERT(soundpack.has_value());
  TEST_ASSERT(soundpack->Name() == "Test Soundpack");
  TEST_ASSERT(soundpack->MappedKeyCount() == 2);

  const auto sound_a = soundpack->SoundFor(30);
  TEST_ASSERT(sound_a.has_value());
  TEST_ASSERT(sound_a->filename() == "a.wav");

  const auto sound_s = soundpack->SoundFor(31);
  TEST_ASSERT(sound_s.has_value());
  TEST_ASSERT(sound_s->filename() == "s.wav");

  const auto sound_unmapped = soundpack->SoundFor(999);
  TEST_ASSERT(!sound_unmapped.has_value());
  return true;
}

extern bool TestSoundpackLoadMissingConfig() {
  const auto soundpack =
    keywave::SoundPack::Load("/tmp/non_existent_soundpack_dir");
  TEST_ASSERT(!soundpack.has_value());
  return true;
}

extern bool TestSoundpackLoadMalformedJson() {
  TempSoundpack pack("soundpack_test_malformed", "{ invalid json content");
  const auto soundpack = keywave::SoundPack::Load(pack.dir);
  TEST_ASSERT(!soundpack.has_value());
  return true;
}

extern bool TestSoundpackLoadMissingDefines() {
  TempSoundpack pack("soundpack_test_nodefines",
                     R"({"name": "No defines pack"})");
  const auto soundpack = keywave::SoundPack::Load(pack.dir);
  TEST_ASSERT(!soundpack.has_value());
  return true;
}

extern void RunSoundpackTests() {
  RUN_TEST(TestSoundpackLoadValid);
  RUN_TEST(TestSoundpackLoadMissingConfig);
  RUN_TEST(TestSoundpackLoadMalformedJson);
  RUN_TEST(TestSoundpackLoadMissingDefines);
}
