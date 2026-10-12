#ifndef KEYWAVE_SOUNDPACK_HPP
#define KEYWAVE_SOUNDPACK_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace keywave {

class SoundPack {
 public:
  [[nodiscard]] static std::optional<SoundPack> Load(const std::filesystem::path& dir);
  [[nodiscard]] std::optional<std::filesystem::path> SoundFor(int keycode) const;
  [[nodiscard]] const std::string& Name() const noexcept { return name_; }
  [[nodiscard]] std::size_t MappedKeyCount() const noexcept { return defines_.size(); }

 private:
  std::filesystem::path dir_;
  std::string name_;
  std::unordered_map<int, std::string> defines_;
  std::optional<std::string> default_sound_;
};

}  // namespace keywave

#endif  // KEYWAVE_SOUNDPACK_HPP
