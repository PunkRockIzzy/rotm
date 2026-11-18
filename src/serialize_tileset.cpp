#include "serialize_tileset.hpp"
#include "utility.hpp"

#include <cstddef>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <optional>
#include <string>

/*
    Format:
    Tile size: signed int, 4 bytes (0x0 - 0x3)
    Tile count (usableTiles.size()): unsigned int, 8 bytes (0x4 - 0xB)
    Usable tiles: packed bits, tileCount/8 rounded up (0xC - eof)

    TODO: checksum to verify tileset image has not been altered since last write
*/

namespace rotm
{
std::optional<std::string> write_tileset_metadata(const Tileset &tileset, const std::filesystem::path &tilesetPath)
{
    const std::string filename = std::format("{}{}", tilesetPath.string(), TILESET_METADATA_EXT);
    std::ofstream file{filename, std::ios_base::binary};

    if (!file.good())
        return std::format("Failed to open file output stream. (filename={})", filename);

    std::size_t filepos = 0;

    TilesetMetadata fileData = {.tileSize = tileset.get_tile_size(),
                                .tileCount = tileset.get_tile_count(),
                                .usableTilesPacked = pack_bitset(tileset.get_usable_tiles())};

    if (!write_to_stream(file, fileData.tag, &filepos))
        return std::format("Failed to write tag to file. (filename={}, filepos={:#010x})", filename, filepos);

    if (!write_to_stream(file, fileData.tileSize, &filepos))
        return std::format("Failed to write tile size to file. (filename={}, filepos={:#010x})", filename, filepos);

    if (!write_to_stream(file, fileData.tileCount, &filepos))
        return std::format("Failed to write tile count to file. (filename={}, filepos={:#010x})", filename, filepos);

    for (const auto &byte : fileData.usableTilesPacked)
    {
        if (!write_to_stream(file, byte, &filepos))
            return std::format("Failed to write usable tiles to file. (filename={}, filepos={:#010x})", filename,
                               filepos);
    }

    file.flush();

    if (!file.good())
        return std::format("Failed to flush output stream. (filename={}, filepos={:#010x})", filename, filepos);

    file.close();

    if (!file.good())
        return std::format("Failed to close output stream. (filename={}, filepos={:#010x})", filename, filepos);

    return std::nullopt;
}

std::expected<TilesetMetadata, std::string> read_tileset_metadata(const std::filesystem::path &tilesetPath)
{
    using err_t = std::unexpected<std::string>;

    const std::string filename = std::format("{}{}", tilesetPath.string(), TILESET_METADATA_EXT);
    std::ifstream file{filename};

    if (!file.good())
        return err_t{std::format("Failed to open file input stream. (filename={})", filename)};

    std::size_t filepos = 0;

    TilesetMetadata out;

    if (!read_from_stream(file, out.tag, &filepos))
        return err_t{std::format("Failed to read tag. (filename={}, filepos={:#010x})", filename, filepos)};

    if (std::memcmp(out.tag, "TMD", 4))
        return err_t{std::format("File tag is incorrect. (filename={}, filepos={:#010x})", filename, filepos)};

    if (!read_from_stream(file, out.tileSize, &filepos))
        return err_t{std::format("Failed to read tile size. (filename={}, filepos={:#010x})", filename, filepos)};

    if (!read_from_stream(file, out.tileCount, &filepos))
        return err_t{std::format("Failed to read tile count. (filename={}, filepos={:#010x})", filename, filepos)};

    const std::size_t byteCount = out.tileCount % 8 == 0 ? out.tileCount / 8 : (out.tileCount / 8) + 1;

    out.usableTilesPacked.reserve(byteCount);

    for (std::size_t i = 0; i < byteCount; ++i)
    {
        std::uint8_t readByte = 0;

        if (!read_from_stream(file, readByte, &filepos))
            return err_t{
                std::format("Failed to read usable tiles. (filename={}, filepos={:#010x})", filename, filepos)};

        out.usableTilesPacked.push_back(readByte);
    }

    file.close();

    if (!file.good())
        return err_t{std::format("Failed to close input stream. (filename={}, filepos={:#010x})", filename, filepos)};

    return out;
}
} // namespace rotm
