#include "keywave/soundpack.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace keywave {

    std::optional<SoundPack> SoundPack::load(const std::filesystem::path& dir) {
        const std::filesystem::path configPath = dir / "config.json";

        std::ifstream file(configPath);
        if (!file) {
            std::cerr << "Cannot open " << configPath.string() << "\n";
            return std::nullopt;
        }

        nlohmann::json j;
        try {
            file >> j;
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Failed to parse " << configPath.string() << ": " << e.what() << "\n";
            return std::nullopt;
        }

        if (!j.contains("defines") || !j["defines"].is_object()) {
            std::cerr << configPath.string() << " has no \"defines\" object.\n";
            return std::nullopt;
        }

        SoundPack pack;
        pack.m_dir = dir;
        pack.m_name = j.value("name", std::string("unnamed soundpack"));

        if (j.contains("sound") && j["sound"].is_string()) {
            pack.m_defaultSound = j["sound"].get<std::string>();
        }

        for (const auto& [codeStr, filename] : j["defines"].items()) {
            if (!filename.is_string())
                continue;
            try {
                const int code = std::stoi(codeStr);
                pack.m_defines.emplace(code, filename.get<std::string>());
            } catch (const std::exception&) {
                std::cerr << "Skipping non-numeric key in defines: \"" << codeStr << "\"\n";
            }
        }

        std::cout << "Loaded soundpack \"" << pack.m_name << "\" (" << pack.m_defines.size() << " keys mapped)\n";
        return pack;
    }

    std::optional<std::filesystem::path> SoundPack::soundFor(int keyCode) const {
        if (auto it = m_defines.find(keyCode); it != m_defines.end()) {
            return m_dir / it->second;
        }
        // NOTE: m_defaultSound ("sound.wav" in mechvibes packs) is a composite
        // audio track containing all key sounds stitched together. It must NOT
        // be used as a per-key fallback — doing so plays the entire track.
        // Unmapped keys are intentionally silent.
        return std::nullopt;
    }

} // namespace keywave
