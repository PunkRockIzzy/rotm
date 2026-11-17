#include "tileset.hpp"
#include "raylib.h"
#include "serialize_tileset.hpp"
#include "tilemap.hpp"
#include "utility.hpp"


namespace rotm
{
Tileset::Tileset(std::shared_ptr<TextureResource> &&atlasTexture, const TilesetMetadata &metadata)
    : m_tileSize(metadata.tileSize), m_atlas(std::move(atlasTexture)),
      m_usableTiles(unpack_bitset(metadata.usableTilesPacked))
{
    if (std::size_t diff = m_usableTiles.size() - metadata.tileCount)
    {
        for (std::size_t i = 0; i < diff; ++i)
        {
            m_usableTiles.pop_back();
        }
    }

    generate_uvs();
}

bool Tileset::has_tile(TileData::IDType id) const
{
    const std::size_t idx = static_cast<std::size_t>(id);

    if (idx >= m_uvs.size())
        return false;

    return m_usableTiles[idx];
}

const Vector2 *Tileset::get_uv(TileData::IDType id) const
{
    if (!has_tile(id))
        return nullptr;

    return &m_uvs[id];
}

Rectangle Tileset::get_uv_rec(TileData::IDType id) const
{
    if (!has_tile(id))
        return {0.0f, 0.0f, 0.0f, 0.0f};

    const Vector2 &uv = m_uvs[id];
    const float tileSizeF = static_cast<float>(m_tileSize);

    return Rectangle{.x = uv.x, .y = uv.y, .width = tileSizeF, .height = tileSizeF};
}

void Tileset::generate_uvs()
{
    const Texture2D &atlas = get_atlas();

    const int atlasTileWidth = atlas.width / m_tileSize;
    const int atlasTileHeight = atlas.height / m_tileSize;
    const float tileSizeF = static_cast<float>(m_tileSize);

    m_uvs.reserve(static_cast<std::size_t>(atlasTileWidth * atlasTileHeight));

    for (int y = 0; y < atlasTileHeight; ++y)
    {
        for (int x = 0; x < atlasTileWidth; ++x)
        {
            m_uvs.emplace_back(static_cast<float>(x) * tileSizeF, static_cast<float>(y) * tileSizeF);
        }
    }
}
} // namespace rotm
