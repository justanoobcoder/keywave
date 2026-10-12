#include "keywave/soundpack.hpp"

#include <charconv>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

namespace keywave {

std::optional<SoundPack> SoundPack::Load(const std::filesystem::path& dir) {
  const std::filesystem::path config_path = dir / "config.json";

  std::ifstream file(config_path);
  if (!file) {
    std::cerr << "Cannot open " << config_path.string() << "\n";
    return std::nullopt;
  }

  const nlohmann::json j = nlohmann::json::parse(file, nullptr, false);
  if (j.is_discarded()) {
    std::cerr << "Failed to parse " << config_path.string() << "\n";
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

  for (const auto& [code_str, filename] : j["defines"].items()) {
    if (!filename.is_string()) {
      continue;
    }
    int code = 0;
    const auto [ptr, ec] =
      std::from_chars(code_str.data(), code_str.data() + code_str.size(), code);
    if (ec != std::errc{} || ptr != code_str.data() + code_str.size()) {
      std::cerr << "Skipping non-numeric key in defines: \"" << code_str << "\"\n";
      continue;
    }
    pack.defines_.emplace(code, filename.get<std::string>());
  }

  std::cout << "Loaded soundpack \"" << pack.name_ << "\" (" << pack.defines_.size()
            << " keys mapped)\n";
  return pack;
}

std::optional<std::filesystem::path> SoundPack::SoundFor(int keycode) const {
  if (auto it = defines_.find(keycode); it != defines_.end()) {
    return dir_ / it->second;
  }

  return std::nullopt;
}

}  // namespace keywave
