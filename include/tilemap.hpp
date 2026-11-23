#pragma once

#include "vec2i.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>

namespace rotm
{
using ChunkCoord = Vec2I;
using LocalTileCoord = Vec2I;

struct GlobalTileCoord
{
    ChunkCoord chunk;
    LocalTileCoord tile;

    constexpr auto operator<=>(const GlobalTileCoord &other) const noexcept = default;
};

struct TileData
{
    using IDType = int;

    static constexpr IDType ID_NULL = -1;

    IDType id = ID_NULL;

    bool operator==(const TileData &other) const noexcept = default;
};

class TilemapChunk
{
  public: /* Static properties */
    static constexpr std::size_t SIZE = 32;

  public: /* Types */
    using Data = std::array<std::array<TileData, SIZE>, SIZE>;

  public: /* Methods */
    TilemapChunk() = default;
    ~TilemapChunk() = default;

    TilemapChunk(const TilemapChunk &other) : m_data(other.m_data), m_tileCount(other.m_tileCount), m_dirty(true)
    {
    }

    TilemapChunk(TilemapChunk &&other) noexcept
        : m_data(std::move(other.m_data)), m_tileCount(other.m_tileCount), m_dirty(true)
    {
    }

    TilemapChunk &operator=(const TilemapChunk &other)
    {
        m_data = other.m_data;
        m_tileCount = other.m_tileCount;
        m_dirty = true;

        return *this;
    }

    TilemapChunk &operator=(TilemapChunk &&other) noexcept
    {
        m_data = std::move(other.m_data);
        m_tileCount = other.m_tileCount;
        m_dirty = true;

        return *this;
    }

    const Data &get_data() const
    {
        return m_data;
    }

    std::size_t get_tile_count() const
    {
        return m_tileCount;
    }
    bool empty() const
    {
        return m_tileCount == 0;
    }

    bool get_dirty() const
    {
        return m_dirty;
    }
    void set_dirty(bool dirty)
    {
        m_dirty = dirty;
    }

    const TileData &get_tile(LocalTileCoord coord) const;
    void set_tile(LocalTileCoord coord, const TileData &source);

    void clear();

  private: /* Fields */
    Data m_data;
    std::size_t m_tileCount = 0;
    bool m_dirty = true;
};

class Tilemap
{
  public:
    using Data = std::unordered_map<ChunkCoord, TilemapChunk>;

    const Data &get_data() const
    {
        return m_data;
    }

    TilemapChunk &operator[](ChunkCoord coord)
    {
        return m_data[coord];
    }

    TilemapChunk *get_chunk(ChunkCoord coord);
    const TilemapChunk *get_chunk(ChunkCoord coord) const;
    void set_chunk(ChunkCoord coord, const TilemapChunk &source);
    void set_chunk(ChunkCoord coord, TilemapChunk &&source) noexcept;
    void erase_chunk(ChunkCoord coord);
    std::size_t get_chunk_count() const
    {
        return m_data.size();
    }

    const TileData *get_tile(GlobalTileCoord coord) const;
    void set_tile(GlobalTileCoord coord, const TileData &source);
    std::size_t get_tile_count() const;

    void stress_test(const TileData &tile);

  private:
    Data m_data;
};
} // namespace rotm

namespace std
{
template <> struct hash<rotm::GlobalTileCoord>
{
    static std::size_t operator()(const rotm::GlobalTileCoord &coord) noexcept
    {
        std::size_t seed = 0;

        hash_combine(seed, coord.chunk.x);
        hash_combine(seed, coord.chunk.y);
        hash_combine(seed, coord.tile.x);
        hash_combine(seed, coord.tile.y);

        return seed;
    }

  private:
    template <typename T> static void hash_combine(std::size_t &seed, const T &value) noexcept
    {
        seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
};
} // namespace std
