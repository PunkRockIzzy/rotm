#pragma once

#include "scene.hpp"
#include "serialize_tileset.hpp"

#include <expected>
#include <optional>
#include <string_view>

namespace rotm
{
static constexpr std::string_view SCENE_ROOT = "scenes";

static constexpr std::string_view TILEMAP_FILE_NAME = "tilemap.rtm";

struct SceneFileData
{
    Tilemap tilemap;
    TilesetMetadata tilesetMetadata;
    std::filesystem::path tilesetPath;
};

std::optional<std::string> write_scene(const Scene &scene, std::string_view sceneName);
std::expected<SceneFileData, std::string> read_scene(std::string_view sceneName);
} // namespace rotm
