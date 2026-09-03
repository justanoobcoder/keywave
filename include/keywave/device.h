#pragma once

#include <filesystem>
#include <optional>

namespace keywave {

[[nodiscard]] std::optional<std::filesystem::path> findMouseDevice();

} // namespace keywave
