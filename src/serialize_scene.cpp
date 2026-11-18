#include "serialize_scene.hpp"
#include "scene.hpp"
#include "serialize_tilemap.hpp"
#include "serialize_tileset.hpp"
#include "tilemap.hpp"
#include "tileset.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

namespace rotm
{
namespace fs = std::filesystem;

std::optional<std::string> write_scene(const Scene &scene, std::string_view sceneName)
{
    const fs::path sceneRoot = SCENE_ROOT;

    if (!fs::is_directory(sceneRoot))
        fs::create_directory(sceneRoot);

    const fs::path sceneFolder = sceneRoot / sceneName;

    if (!fs::is_directory(sceneFolder))
    {
        if (fs::is_regular_file(sceneFolder))
            return std::format("There is already a file with that name. ({})", sceneFolder.string());

        fs::create_directory(sceneFolder);
    }

    const fs::path tilemapFileName = sceneFolder / "tilemap.rtm";

    auto tilemapResult = write_tilemap(scene.tilemap, tilemapFileName, scene.tilesetFileName);

    if (tilemapResult)
        return tilemapResult;

    return std::nullopt;
}

std::expected<SceneFileData, std::string> read_scene(std::string_view sceneName)
{
    using err_t = std::unexpected<std::string>;

    SceneFileData out;

    fs::path sceneRoot = SCENE_ROOT;

    if (!fs::is_directory(sceneRoot))
    {
        return err_t{std::format("Missing scenes folder. ({})", sceneRoot.string())};
    }

    const fs::path sceneFolder = sceneRoot / sceneName;

    if (!fs::is_directory(sceneFolder))
    {
        return err_t{std::format("No scene exists with that name. ({})", sceneFolder.string())};
    }

    const fs::path tilemapFileName = sceneFolder / TILEMAP_FILE_NAME;

    if (!fs::is_regular_file(tilemapFileName))
    {
        return err_t{std::format("Scene is missing tilemap. ({})", tilemapFileName.string())};
    }

    auto readTilemapResult = read_tilemap(tilemapFileName);

    if (!readTilemapResult.has_value())
    {
        return err_t{std::move(readTilemapResult.error())};
    }

    ReadTilemapOutput readTilemapOutput = std::move(*readTilemapResult);

    out.tilemap = std::move(readTilemapOutput.tilemap);
    out.tilesetPath = readTilemapOutput.tilesetFilename;

    auto readTilesetMetaResult = read_tileset_metadata(out.tilesetPath);

    if (!readTilesetMetaResult.has_value())
    {
        return err_t{std::move(readTilesetMetaResult.error())};
    }

    out.tilesetMetadata = std::move(*readTilesetMetaResult);

    return out;
}
} // namespace rotm
