#pragma once

#include "logger.hpp"
#include "tilemap.hpp"
#include "tileset.hpp"
#include "time.hpp"

#include <cstddef>
#include <unordered_map>

#include <raylib.h>

namespace rotm
{
	class TilemapRenderer
	{
	public:
		TilemapRenderer() = default;
	
		TilemapRenderer(const TilemapRenderer&) = delete;
		TilemapRenderer& operator=(const TilemapRenderer&) = delete;
		
		TilemapRenderer(TilemapRenderer&&) noexcept = default;
		TilemapRenderer& operator=(TilemapRenderer&&) noexcept = default;
	
		void update(
			const Tileset& tileset,
			Tilemap& tilemap,
			const Camera2D& camera
		);
		
		void draw(int tileSize);

		static void draw_tile(
			const Tileset& tileset,
			GlobalTileCoord coord,
			const TileData& tile,
			Color tint = WHITE
		);

		std::size_t get_chunks_drawn_count() const
		{
			return m_chunksDrawnCount;
		}

		std::size_t get_visible_chunk_count() const
		{
			return m_visibleChunks.size();
		}

		bool get_show_chunk_borders() const
		{
			return m_showChunkBorders;
		}
		
		void set_show_chunk_borders(bool showChunkBorders)
		{
			m_showChunkBorders = showChunkBorders;
		}

	private:
		static constexpr std::size_t CHUNK_CACHE_MAX_SIZE = 128;
		
		struct CachedChunk
		{
			RenderTexture renderTexture;
			time::Point timeCreated = time::Clock::now();
			time::Point timeLastSeen = timeCreated;

			CachedChunk(int tileSize);
			~CachedChunk();

			CachedChunk() = delete;

			CachedChunk(const CachedChunk&) = delete;
			CachedChunk& operator=(const CachedChunk&) = delete;
			
			CachedChunk(CachedChunk&&) = default;
			CachedChunk& operator=(CachedChunk&&) = default;
			
			void render(const Tileset& tileset, TilemapChunk& chunk);
		};

		void calculate_visible_chunks(int tileSize, const Camera2D& camera);
		void build_render_list(const Tilemap& tilemap);
		void discard_old_chunks();

		void draw_chunk(const ChunkCoord& chunkCoord, const CachedChunk& chunk, int tileSize);

		std::unordered_map<ChunkCoord, CachedChunk> m_chunkCache;
		std::vector<ChunkCoord> m_visibleChunks;
		std::vector<ChunkCoord> m_renderList;

		std::size_t m_chunksDrawnCount = 0;
	
		bool m_showChunkBorders = false;
	};
} // namespace rotm
