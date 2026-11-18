#pragma once

#include "tileset.hpp"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rotm
{
static constexpr std::string_view TILESET_METADATA_EXT = ".meta";

struct TilesetMetadata
{
    char tag[4] = "TMD";
    std::int32_t tileSize = 0;
    std::uint64_t tileCount = 0;
    std::vector<std::uint8_t> usableTilesPacked;
};

std::optional<std::string> write_tileset_metadata(const Tileset &tileset, const std::filesystem::path &tilesetPath);
std::expected<TilesetMetadata, std::string> read_tileset_metadata(const std::filesystem::path &tilesetPath);
} // namespace rotm
