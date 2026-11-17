#pragma once

#include "exception.hpp"
#include "resource.hpp"

#include <filesystem>
#include <raylib.h>

namespace rotm
{
	class TextureResource final : public Resource<Texture2D>
	{
	public:
		TextureResource(const std::filesystem::path& path) :
			Resource<Texture2D>(path)
		{
			const std::string pathStr = m_path.string();

			m_data = LoadTexture(pathStr.c_str());

			if (!IsTextureValid(m_data))
				throw exception::ResourceInvalid(std::filesystem::relative(m_path));
		}

		~TextureResource() override
		{
			UnloadTexture(m_data);
		}
	};
}
