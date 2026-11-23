#pragma once

#include "renderer_tilemap.hpp"
#include "tilemap.hpp"
#include "tileset.hpp"

#include <raylib.h>

namespace rotm
{
struct Scene
{
    Scene() = delete;

    // Create scene with an empty tilemap.
    Scene(Tileset&& _tileset, const std::filesystem::path& _tilesetFileName)
        : tileset(std::move(_tileset)), tilesetFileName(_tilesetFileName), tilemap()
    {
    }

    // Create scene with an existing tilemap.
    Scene(Tileset&& _tileset, const std::filesystem::path& _tilesetFileName, Tilemap&& _tilemap)
        : tileset(std::move(_tileset)), tilesetFileName(_tilesetFileName), tilemap(std::move(_tilemap))
    {
    }

    Camera2D camera;

    Tileset tileset;
    std::filesystem::path tilesetFileName;
    // TODO: Scene node tree, convert tilemap to node
    Tilemap tilemap;

    struct Render
    {
        TilemapRenderer tilemap;
    } render;
};
} // namespace rotm
