#pragma once

#include <filesystem>
#include <optional>

namespace keywave {

[[nodiscard]] std::optional<std::filesystem::path>
findInputDevice(int requiredKeyCode);

[[nodiscard]] std::optional<std::filesystem::path> findMouseDevice();
[[nodiscard]] std::optional<std::filesystem::path> findKeyboardDevice();

} // namespace keywave
