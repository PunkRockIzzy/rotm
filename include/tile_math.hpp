#pragma once

#include "tilemap.hpp"
#include "vec2i.hpp"

#include <cmath>
#include <raylib.h>

namespace rotm
{
// A tile's position in the overall tilemap grid. (chunk-agnostic position)
using AbsoluteTileCoord = Vec2I;

/**
 * @brief Convert a tile coordinate to a screen/world space coordinate.
 */
inline Vector2 tile_to_point(GlobalTileCoord coord, int tileSize)
{
    const float tileSizeF = static_cast<float>(tileSize);

    const float x = (static_cast<float>(coord.chunk.x) * static_cast<float>(TilemapChunk::SIZE) * tileSizeF) +
                    (static_cast<float>(coord.tile.x) * tileSizeF);

    const float y = (static_cast<float>(coord.chunk.y) * static_cast<float>(TilemapChunk::SIZE) * tileSizeF) +
                    (static_cast<float>(coord.tile.y) * tileSizeF);

    return {x, y};
}

/**
 * @brief Convert a screen/world space coordinate to a tile coordinate.
 */
inline GlobalTileCoord point_to_tile(Vector2 point, int tileSize)
{
    using std::floor;

    const float chunkSizePixels = static_cast<float>(TilemapChunk::SIZE) * static_cast<float>(tileSize);

    const ChunkCoord chunk = {.x = static_cast<int>(floor(point.x / chunkSizePixels)),
                              .y = static_cast<int>(floor(point.y / chunkSizePixels))};

    const LocalTileCoord tile = {static_cast<int>(floor(point.x / static_cast<float>(tileSize))) -
                                     (chunk.x * static_cast<int>(TilemapChunk::SIZE)),

                                 static_cast<int>(floor(point.y / static_cast<float>(tileSize))) -
                                     (chunk.y * static_cast<int>(TilemapChunk::SIZE))};

    return {chunk, tile};
};

/**
 * @brief Convert a global tile coordinate to a chunk-agnostic coordinate.
 */
inline AbsoluteTileCoord global_to_absolute(GlobalTileCoord coord)
{
    return {.x = (coord.chunk.x * static_cast<int>(TilemapChunk::SIZE)) + coord.tile.x,
            .y = (coord.chunk.y * static_cast<int>(TilemapChunk::SIZE)) + coord.tile.y};
}

/**
 * @brief Convert a chunk-agnostic coordinate to a global tile coordinate.
 */
inline GlobalTileCoord absolute_to_global(AbsoluteTileCoord coord)
{
    static auto floorDiv = [](int a, int b) -> int {
        const int quotient = a / b;
        const int remainder = a % b;

        return quotient - (remainder < 0);
    };

    const int chunkSize = static_cast<int>(TilemapChunk::SIZE);

    const ChunkCoord chunk = {.x = floorDiv(coord.x, chunkSize), .y = floorDiv(coord.y, chunkSize)};

    const LocalTileCoord tile = {.x = coord.x - (chunk.x * chunkSize), .y = coord.y - (chunk.y * chunkSize)};

    return {chunk, tile};
}

template <typename Fn> void for_each_tile_on_line(AbsoluteTileCoord start, AbsoluteTileCoord end, Fn &&callback)
{
    using std::abs;

    int dx = abs(end.x - start.x);
    int dy = abs(end.y - start.y);

    const int sx = (start.x < end.x) ? 1 : -1;
    const int sy = (start.y < end.y) ? 1 : -1;

    int error = dx - dy;

    while (true)
    {
        callback(start);

        if (start == end)
            break;

        const int error2 = 2 * error;

        if (error2 > -dy)
        {
            error -= dy;
            start.x += sx;
        }

        if (error2 < dx)
        {
            error += dx;
            start.y += sy;
        }
    }
}
} // namespace rotm
