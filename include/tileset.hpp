#pragma once

#include "resource_texture.hpp"
#include "tilemap.hpp"

#include <memory>
#include <vector>

namespace rotm
{
struct TilesetMetadata;

class Tileset
{
public:
    Tileset() = delete;

    Tileset(std::shared_ptr<TextureResource>&& atlasTexture, const TilesetMetadata& metadata);

    Tileset(const Tileset&) = delete;
    Tileset(Tileset&&) = default;

    Tileset& operator=(const Tileset&) = delete;
    Tileset& operator=(Tileset&&) = default;

    ~Tileset() = default;

    int get_tile_size() const
    {
        return m_tileSize;
    }

    const Texture2D& get_atlas() const
    {
        return m_atlas->get();
    }

    std::size_t get_tile_count() const
    {
        return m_usableTiles.size();
    }

    const std::vector<bool>& get_usable_tiles() const
    {
        return m_usableTiles;
    }

    bool has_tile(TileData::IDType id) const;
    const Vector2* get_uv(TileData::IDType id) const;
    Rectangle get_uv_rec(TileData::IDType id) const;

private:
    void generate_uvs();

    int m_tileSize;

    std::shared_ptr<TextureResource> m_atlas;
    std::vector<bool> m_usableTiles;

    std::vector<Vector2> m_uvs;
};
} // namespace rotm
