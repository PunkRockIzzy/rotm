#include "tilemap.hpp"
#include "tracy/Tracy.hpp"

namespace rotm
{
// yes i'm aware some of these can throw, it's intentional

const TileData &TilemapChunk::get_tile(LocalTileCoord coord) const
{
    ZoneScopedN("TilemapChunk::get_tile");

    return m_data.at(coord.y).at(coord.x);
}

void TilemapChunk::set_tile(LocalTileCoord coord, const TileData &source)
{
    TileData &target = m_data.at(coord.y).at(coord.x);

    if (target == source)
        return;

    m_dirty = true;

    const bool sourceEmpty = source.id == TileData::ID_NULL;
    const bool targetEmpty = target.id == TileData::ID_NULL;

    if (sourceEmpty and !targetEmpty)
        --m_tileCount;
    else if (!sourceEmpty and targetEmpty)
        ++m_tileCount;

    target = source;
}

void TilemapChunk::clear()
{
    for (auto &row : m_data)
        row.fill(TileData{});

    m_tileCount = 0;
    m_dirty = true;
}

TilemapChunk *Tilemap::get_chunk(ChunkCoord coord)
{
    ZoneScopedN("Tilemap::get_chunk");

    const auto it = m_data.find(coord);

    if (it == m_data.end())
        return nullptr;

    return &it->second;
}

const TilemapChunk *Tilemap::get_chunk(ChunkCoord coord) const
{
    ZoneScopedN("Tilemap::get_chunk");
    const auto it = m_data.find(coord);

    if (it == m_data.end())
        return nullptr;

    return &it->second;
}

void Tilemap::set_chunk(ChunkCoord coord, const TilemapChunk &source)
{
    m_data[coord] = source;
}

void Tilemap::set_chunk(ChunkCoord coord, TilemapChunk &&source) noexcept
{
    m_data[coord] = std::move(source);
}

void Tilemap::erase_chunk(ChunkCoord coord)
{
    m_data.erase(coord);
}

const TileData *Tilemap::get_tile(GlobalTileCoord coord) const
{
    ZoneScopedN("Tilemap::get_tile");

    const TilemapChunk *chunk = get_chunk(coord.chunk);

    if (!chunk)
        return nullptr;

    return &chunk->get_tile(coord.tile);
}

void Tilemap::set_tile(GlobalTileCoord coord, const TileData &source)
{
    TilemapChunk &chunk = m_data[coord.chunk];

    chunk.set_tile(coord.tile, source);

    if (chunk.get_tile_count() == 0)
        m_data.erase(coord.chunk);
}

std::size_t Tilemap::get_tile_count() const
{
    ZoneScopedN("Tilemap::get_tile_count");

    std::size_t out = 0;

    for (const auto &[chunkCoord, chunk] : m_data)
    {
        (void)chunkCoord;
        out += chunk.get_tile_count();
    }

    return out;
}

void Tilemap::stress_test(const TileData &tile)
{
    static constexpr int MONOLITH_START = -499;
    static constexpr int MONOLITH_END = 500;

    for (int chunkY = MONOLITH_START; chunkY <= MONOLITH_END; ++chunkY)
    {
        for (int chunkX = MONOLITH_START; chunkX <= MONOLITH_END; ++chunkX)
        {
            TilemapChunk &chunk = m_data[{chunkX, chunkY}];

            for (int tileY = 0; tileY < static_cast<int>(TilemapChunk::SIZE); ++tileY)
            {
                for (int tileX = 0; tileX < static_cast<int>(TilemapChunk::SIZE); ++tileX)
                {
                    chunk.set_tile({tileX, tileY}, tile);
                }
            }
        }
    }
}
} // namespace rotm
