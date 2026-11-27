#pragma once

#include "tilemap.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace rotm
{
static constexpr std::string_view TILEMAP_EXT = ".rtm";

struct ReadTilemapOutput
{
    Tilemap tilemap;
    std::filesystem::path tilesetFilename;
};

std::optional<std::string> write_tilemap(const Tilemap& tilemap, const std::filesystem::path& path,
                                         const std::filesystem::path& tilesetPath);

std::expected<ReadTilemapOutput, std::string> read_tilemap(const std::filesystem::path& path);
} // namespace rotm
