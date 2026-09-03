#include "test_framework.h"

#include "keywave/soundpack.h"

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
    const auto    soundpack = keywave::SoundPack::load(pack.dir);
    TEST_ASSERT(!soundpack.has_value());
    return true;
}

bool test_soundpack_loadMissingDefines() {
    TempSoundpack pack("soundpack_test_nodefines", R"({"name": "No defines pack"})");
    const auto    soundpack = keywave::SoundPack::load(pack.dir);
    TEST_ASSERT(!soundpack.has_value());
    return true;
}

void runSoundpackTests() {
    RUN_TEST(test_soundpack_loadValid);
    RUN_TEST(test_soundpack_loadMissingConfig);
    RUN_TEST(test_soundpack_loadMalformedJson);
    RUN_TEST(test_soundpack_loadMissingDefines);
}
