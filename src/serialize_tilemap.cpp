#include "serialize_tilemap.hpp"
#include "tilemap.hpp"
#include "utility.hpp"

#include <cstdint>
#include <cstring>
#include <expected>
#include <fstream>
#include <ios>
#include <optional>
#include <type_traits>

namespace rotm
{
	#pragma pack(push, 1)

	struct FileHeader	// 16 bytes
	{
		char tag[4] = "RTM";
		uint16_t formatVersion = 1;
		uint16_t flags = 0x0000;		// currently unused
		uint64_t chunkCount = 0;
	};

	static_assert(
		TilemapChunk::SIZE * TilemapChunk::SIZE < UINT32_MAX,
		"Update chunk serialization, chunks too large to represent tile count."
	);
	struct ChunkHeader
	{
		char tag[4] = "CHK";
		int32_t x = 0;
		int32_t y = 0;
		uint32_t tileCount = 0;
	};

	static_assert(
		std::is_same_v<TileData::IDType, int32_t>,
		"Update tile serialization, tile ID type mismatch."
	);
	struct TileFileData
	{
		char tag[4] = "TIL";
		uint32_t x = 0;
		uint32_t y = 0;
		int32_t id = TileData::ID_NULL;
	};

	#pragma pack(pop)

	std::optional<std::string> write_tilemap(const Tilemap& tilemap, const std::filesystem::path& path, const std::filesystem::path& tilesetPath)
	{
		const std::string filename = path.string();
		const std::string tilesetFilename = tilesetPath.string();
		std::ofstream file{ path, std::ios_base::binary };

		if (!file.good())
			return std::format(
				"Failed to open file output stream. (filename={})",
				filename
			);

		const Tilemap::Data& mapData = tilemap.get_data();
		
		FileHeader fileHeader;
		fileHeader.chunkCount = mapData.size();

		std::size_t filepos = 0;

		if (!write_to_stream(file, fileHeader, &filepos))
			return std::format(
				"Failed to write file header. (filename={}, filepos={:#010x})",
				filename, filepos
			);

		for (const auto& [chunkCoord, chunk] : mapData)
		{
			ChunkHeader chunkHeader;
			chunkHeader.x = chunkCoord.x;
			chunkHeader.y = chunkCoord.y;
			chunkHeader.tileCount = static_cast<uint32_t>(chunk.get_tile_count());

			if (!write_to_stream(file, chunkHeader, &filepos))
				return std::format(
					"Failed to write chunk header. (filename={}, filepos={:#010x})",
					filename, filepos
				);

			for (int y = 0; y < static_cast<int>(TilemapChunk::SIZE); ++y)
			{
				for (int x = 0; x < static_cast<int>(TilemapChunk::SIZE); ++x)
				{
					const TileData& tile = chunk.get_tile({ x, y });

					if (tile.id == TileData::ID_NULL) continue;

					TileFileData tileFileData;
					tileFileData.x = static_cast<uint32_t>(x);
					tileFileData.y = static_cast<uint32_t>(y);
					tileFileData.id = tile.id;

					if (!write_to_stream(file, tileFileData, &filepos))
						return std::format(
							"Failed to write tile data. (filename={}, filepos={:#010x})",
							filename, filepos
						);
				}
			}
		}

		if (!write_to_stream(file, tilesetFilename.length(), &filepos))
			return std::format(
				"Failed to write tileset filename length. (filename={}, tilesetFilename={}, filepos={:#010x})",
				filename, tilesetFilename, filepos
			);

		for (const char& c : tilesetFilename)
		{
			if (!write_to_stream(file, c, &filepos))
				return std::format(
					"Failed to write tileset filename. (filename={}, tilesetFilename={}, filepos={:#010x})",
					filename, tilesetFilename, filepos
				);
		}

		file.flush();

		if (!file.good())
			return std::format(
				"Failed to flush output stream. (filename={}, filepos={:#010x})",
				filename, filepos
			);
		
		file.close();

		if (!file.good())
			return std::format(
				"Failed to close output stream. (filename={}, filepos={:#010x})",
				filename, filepos
			);

		return std::nullopt;
	}

	std::expected<ReadTilemapOutput, std::string> read_tilemap(const std::filesystem::path& path)
	{
		using err_t = std::unexpected<std::string>;
		
		ReadTilemapOutput out;

		std::ifstream file{ path, std::ios_base::binary };

		if (!file.good()) 
			return err_t{ "Failed to initialize input stream." };

		const std::string filename = path.string();

		std::size_t filepos = 0;

		FileHeader fileHeader;
		if (!read_from_stream(file, fileHeader, &filepos))
			return err_t{ 
				std::format(
					"Failed to read file header. (filename={}, filepos={:#010x})",
					filename,
					filepos
				)
			};

		if (std::memcmp(fileHeader.tag, "RTM", 4))
			return err_t{
				std::format(
					"File header tag is incorrect. (filename={}, filepos={:#010x})",
					filename,
					filepos
				)
			};

		if (fileHeader.formatVersion != FileHeader{}.formatVersion)
			return err_t{ 
				std::format(
					"File format version mismatch. (filename={}, filepos={:#010x})",
					filename,
					filepos
				)
			};

		for (std::size_t i = 0; i < fileHeader.chunkCount; ++i)
		{
			ChunkHeader chunkHeader;
			if (!read_from_stream(file, chunkHeader, &filepos))
				return err_t{ 
					std::format(
						"Failed to read chunk header. (filename={}, filepos={:#010x})",
						filename,
						filepos
					)
				};

			if (std::memcmp(chunkHeader.tag, "CHK", 4))
				return err_t{ 
					std::format(
						"Chunk header tag is incorrect. (filename={}, filepos={:#010x})",
						filename,
						filepos
					)
				};

			TilemapChunk& chunk = out.tilemap[{ chunkHeader.x, chunkHeader.y }];

			for (uint32_t i = 0; i < chunkHeader.tileCount; ++i)
			{
				TileFileData tileFileData;
				if (!read_from_stream(file, tileFileData, &filepos))
					return err_t{ 
						std::format(
							"Failed to read tile data. (filename={}, filepos={:#010x})",
							filename,
							filepos
						)
					};

				if (std::memcmp(tileFileData.tag, "TIL", 4))
					return err_t{ 
						std::format(
							"Tile data tag is incorrect. (filename={}, filepos={:#010x})",
							filename,
							filepos
						)
					};

				const LocalTileCoord tileCoord =
				{
					static_cast<int>(tileFileData.x),
					static_cast<int>(tileFileData.y)
				};

				const TileData tileData =
				{
					.id = tileFileData.id
				};

				chunk.set_tile(tileCoord, tileData);
			}
		}

		std::size_t tilesetFilenameLen = 0;
		if (!read_from_stream(file, tilesetFilenameLen, &filepos))
			return err_t{ 
				std::format(
					"Failed to read tileset filename length. (filename={}, filepos={:#010x})",
					filename,
					filepos
				)
			};

		std::string tilesetFilename;
		tilesetFilename.resize(tilesetFilenameLen, '\0');

		char* tilesetFilenameData = tilesetFilename.data();

		for (std::size_t i = 0; i < tilesetFilenameLen; ++i)
		{
			char c = '\0';

			if (!read_from_stream(file, c, &filepos))
				return err_t{ 
					std::format(
						"Failed to read tileset filename. (filename={}, filepos={:#010x})",
						filename,
						filepos
					)
				};

			tilesetFilenameData[i] = c;
		}

		out.tilesetFilename = tilesetFilename;

		file.close();

		if (!file.good())
			return err_t{ 
				std::format(
					"Failed to close input stream. (filename={}, filepos={:#010x})",
					filename,
					filepos
				)
			};

		return out;
	}
}
