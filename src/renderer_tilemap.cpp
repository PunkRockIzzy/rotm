#include "renderer_tilemap.hpp"
#include "tile_math.hpp"
#include "tilemap.hpp"
#include "time.hpp"
#include "tracy/Tracy.hpp"

#include <map>
#include <unordered_map>
#include <vector>

#include <raylib.h>

namespace rotm
{
void TilemapRenderer::update(const Tileset& tileset, Tilemap& tilemap, const Camera2D& camera)
{
    ZoneScopedN("TilemapRenderer::update");

    const int tileSize = tileset.get_tile_size();

    calculate_visible_chunks(tileSize, camera);
    build_render_list(tilemap);

    for (const ChunkCoord& coord : m_renderList)
    {
        TilemapChunk* chunk = tilemap.get_chunk(coord);
        if (!chunk)
            continue;

        auto it = m_chunkCache.emplace(coord, tileSize).first;

        it->second.render(tileset, *chunk);
    }

    // discard_old_chunks();
}

void TilemapRenderer::draw(int tileSize)
{
    ZoneScopedN("TilemapRenderer::draw");

    m_chunksDrawnCount = 0;

    for (const ChunkCoord& chunkCoord : m_visibleChunks)
    {
        const auto it = m_chunkCache.find(chunkCoord);

        if (it == m_chunkCache.end())
            continue;

        draw_chunk(chunkCoord, it->second, tileSize);
        it->second.timeLastSeen = time::Clock::now();
        ++m_chunksDrawnCount;

        if (showChunkBorders)
        {
            const float chunkSizePixels = static_cast<float>(tileSize) * static_cast<float>(TilemapChunk::SIZE);

            const Vector2 chunkPos = tile_to_point({chunkCoord, {0, 0}}, tileSize);

            const Rectangle rec = {
                .x = chunkPos.x, .y = chunkPos.y, .width = chunkSizePixels, .height = chunkSizePixels};

            DrawRectangleLinesEx(rec, 1.0f, Fade(YELLOW, 0.66f));
        }
    }
}

void TilemapRenderer::draw_tile(const Tileset& tileset, GlobalTileCoord coord, const TileData& tile, Color tint)
{
    ZoneScopedN("TilemapRenderer::draw_tile");

    const int tileSize = tileset.get_tile_size();

    if (tile.id == TileData::ID_NULL)
        return;

    const Vector2 pos = tile_to_point(coord, tileSize);

    const Rectangle srcrec = tileset.get_uv_rec(tile.id);
    const Rectangle dstrec = {pos.x, pos.y, static_cast<float>(tileSize), static_cast<float>(tileSize)};

    DrawTexturePro(tileset.get_atlas(), srcrec, dstrec, {0.0f, 0.0f}, 0.0f, tint);
}

void TilemapRenderer::calculate_visible_chunks(int tileSize, const Camera2D& camera)
{
    ZoneScoped;

    m_visibleChunks.clear();

    const Vector2 screenSize = {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
    const Vector2 minPos = GetScreenToWorld2D({0.0f, 0.0f}, camera);
    const Vector2 maxPos = GetScreenToWorld2D(screenSize, camera);

    const ChunkCoord minChunk = point_to_tile(minPos, tileSize).chunk;
    const ChunkCoord maxChunk = point_to_tile(maxPos, tileSize).chunk;

    const std::size_t visibleChunkCount =
        static_cast<std::size_t>(maxChunk.x - minChunk.x + 1) * static_cast<std::size_t>(maxChunk.y - minChunk.y + 1);

    m_visibleChunks.reserve(visibleChunkCount);

    for (int y = minChunk.y; y <= maxChunk.y; ++y)
    {
        for (int x = minChunk.x; x <= maxChunk.x; ++x)
        {
            m_visibleChunks.emplace_back(x, y);
        }
    }
}

void TilemapRenderer::build_render_list(const Tilemap& tilemap)
{
    ZoneScopedN("TilemapRenderer::build_render_list");

    m_renderList.clear();
    m_renderList.reserve(m_visibleChunks.size());

    for (const ChunkCoord& chunkCoord : m_visibleChunks)
    {
        const TilemapChunk* chunk = tilemap.get_chunk(chunkCoord);

        if (!chunk)
        {
            m_chunkCache.erase(chunkCoord);
            continue;
        }

        if (!chunk->get_dirty())
            continue;

        m_renderList.push_back(chunkCoord);
    }

    // if (m_renderList.empty())
    // {
    // 	for (const auto& [chunkCoord, chunk] : tilemap.get_data())
    // 	{
    // 		if (!chunk.get_dirty()) continue;

    // 		m_renderList.push_back(chunkCoord);
    // 		break;
    // 	}
    // }
}

void TilemapRenderer::discard_old_chunks()
{
    if (m_chunkCache.size() <= CHUNK_CACHE_MAX_SIZE)
        return;

    std::size_t chunksToDelete = m_chunkCache.size() - CHUNK_CACHE_MAX_SIZE; // >= 1

    const time::Point timeNow = time::Clock::now();

    std::multimap<time::Duration, ChunkCoord> chunksByAge;

    for (auto it = m_chunkCache.begin(); it != m_chunkCache.end(); ++it)
    {
        const CachedChunk& cc = it->second;

        const time::Duration chunkAge = timeNow - cc.timeLastSeen;

        chunksByAge.emplace(chunkAge, it->first);
    }

    for (auto it = chunksByAge.rbegin(); it != chunksByAge.rend(); ++it)
    {
        m_chunkCache.erase(it->second);

        --chunksToDelete;
        if (chunksToDelete == 0)
            break;
    }
}

void TilemapRenderer::draw_chunk(const ChunkCoord& chunkCoord, const CachedChunk& chunk, int tileSize)
{
    ZoneScopedN("TilemapRenderer::draw_chunk");

    const Texture2D& texture = chunk.renderTexture.texture;

    const Vector2 chunkPos = tile_to_point({.chunk = chunkCoord, .tile = {0, 0}}, tileSize);

    const Rectangle srcrec = {.x = 0.0f,
                              .y = 0.0f,
                              .width = static_cast<float>(texture.width),
                              .height = -static_cast<float>(texture.height)};

    const float chunkSizePixels = static_cast<float>(TilemapChunk::SIZE) * static_cast<float>(tileSize);

    const Rectangle dstrec = {.x = chunkPos.x, .y = chunkPos.y, .width = chunkSizePixels, .height = chunkSizePixels};

    DrawTexturePro(texture, srcrec, dstrec, {0.0f, 0.0f}, 0.0f, WHITE);
}

TilemapRenderer::CachedChunk::CachedChunk(int tileSize)
{
    const int chunkSizePixels = static_cast<int>(TilemapChunk::SIZE) * tileSize;

    renderTexture = LoadRenderTexture(chunkSizePixels, chunkSizePixels);
}

TilemapRenderer::CachedChunk::~CachedChunk()
{
    if (IsRenderTextureValid(renderTexture))
        UnloadRenderTexture(renderTexture);
}

void TilemapRenderer::CachedChunk::render(const Tileset& tileset, TilemapChunk& chunk)
{
    ZoneScopedN("TilemapRenderer::CachedChunk::render");

    BeginTextureMode(renderTexture);
    ClearBackground(BLANK);

    for (int y = 0; y < static_cast<int>(TilemapChunk::SIZE); ++y)
    {
        for (int x = 0; x < static_cast<int>(TilemapChunk::SIZE); ++x)
        {
            const TileData& tile = chunk.get_tile({x, y});

            TilemapRenderer::draw_tile(tileset, {.chunk = {0, 0}, .tile = {x, y}}, tile);
        }
    }

    EndTextureMode();

    chunk.set_dirty(false);
}
} // namespace rotm
