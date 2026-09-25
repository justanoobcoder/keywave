#include "keywave/soundpack.hpp"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace keywave {

std::optional<SoundPack> SoundPack::Load(const std::filesystem::path& dir) {
  const std::filesystem::path config_path = dir / "config.json";

  std::ifstream file(config_path);
  if (!file) {
    std::cerr << "Cannot open " << config_path.string() << "\n";
    return std::nullopt;
  }

  nlohmann::json j;
  try {
    file >> j;
  } catch (const nlohmann::json::parse_error& e) {
    std::cerr << "Failed to parse " << config_path.string() << ": " << e.what()
              << "\n";
    return std::nullopt;
  }

  if (!j.contains("defines") || !j["defines"].is_object()) {
    std::cerr << config_path.string() << " has no \"defines\" object.\n";
    return std::nullopt;
  }

  SoundPack pack;
  pack.dir_ = dir;
  pack.name_ = j.value("name", std::string("unnamed soundpack"));

  if (j.contains("sound") && j["sound"].is_string()) {
    pack.default_sound_ = j["sound"].get<std::string>();
  }

  for (const auto& [codeStr, filename] : j["defines"].items()) {
    if (!filename.is_string()) continue;
    try {
      const int code = std::stoi(codeStr);
      pack.defines_.emplace(code, filename.get<std::string>());
    } catch (const std::exception&) {
      std::cerr << "Skipping non-numeric key in defines: \"" << codeStr
                << "\"\n";
    }
  }

  std::cout << "Loaded soundpack \"" << pack.name_ << "\" ("
            << pack.defines_.size() << " keys mapped)\n";
  return pack;
}

std::optional<std::filesystem::path> SoundPack::SoundFor(int keyCode) const {
  if (auto it = defines_.find(keyCode); it != defines_.end()) {
    return dir_ / it->second;
  }
  // NOTE: default_sound_ ("sound.wav" in mechvibes packs) is a composite
  // audio track containing all key sounds stitched together. It must NOT
  // be used as a per-key fallback — doing so plays the entire track.
  // Unmapped keys are intentionally silent.
  return std::nullopt;
}

}  // namespace keywave
