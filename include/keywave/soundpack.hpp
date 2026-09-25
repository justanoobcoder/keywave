#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace keywave {

// A mechvibes/wayvibes-compatible "multiple" soundpack: a config.json
// mapping raw evdev key codes (as strings, e.g. "30" for KEY_A) to
// individual sound files, plus an optional catch-all "sound" file used
// for keys that have no explicit entry.
class SoundPack {
 public:
  // Loads <dir>/config.json. Returns std::nullopt on any file-access or
  // parse failure (missing file, malformed JSON, missing "defines").
  [[nodiscard]] static std::optional<SoundPack> Load(
    const std::filesystem::path& dir);

  // Resolves the sound file for a given evdev key code (e.g. KEY_A).
  // Falls back to the pack's default "sound" file if the code has no
  // explicit entry, or std::nullopt if there's no default either.
  [[nodiscard]] std::optional<std::filesystem::path> SoundFor(
    int keyCode) const;

  [[nodiscard]] const std::string& Name() const noexcept { return name_; }
  [[nodiscard]] std::size_t MappedKeyCount() const noexcept {
    return defines_.size();
  }

 private:
  std::filesystem::path dir_;
  std::string name_;
  std::unordered_map<int, std::string> defines_;  // keyCode -> filename
  std::optional<std::string> default_sound_;       // "sound" field, if present
};

}  // namespace keywave
