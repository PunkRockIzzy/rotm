#pragma once

#include "exception.hpp"
#include "logger.hpp"
#include "resource_texture.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace rotm
{
template <typename StoredType> class ResourceCache
{
  public:
    ResourceCache(Logger &loggerRef) : logger(loggerRef)
    {
    }

    std::shared_ptr<StoredType> fetch(const std::filesystem::path &path)
    {
        namespace fs = std::filesystem;

        const fs::path canonPath = fs::weakly_canonical(path);

        if (!fs::is_regular_file(canonPath))
            throw exception::ResourceMissing(path.string());

        auto it = m_cache.find(canonPath);

        if (it == m_cache.end())
        {
            logger.print_info()("Resource not in cache, loading... ({})", path.string());

            it = m_cache.emplace_hint(it, canonPath, std::make_shared<StoredType>(canonPath));
        }

        return it->second;
    }

    std::size_t purge()
    {
        namespace fs = std::filesystem;

        std::size_t eraseCount = 0;

        for (auto it = m_cache.begin(); it != m_cache.end();)
        {
            if (it->second.use_count() == 1)
            {
                logger.print_info()("Resource unused, unloading... ({})", fs::relative(it->first).string());
                it = m_cache.erase(it);
                ++eraseCount;
            }
            else
                ++it;
        }

        return eraseCount;
    }

    void clear()
    {
        m_cache.clear();
    }

  private:
    Logger &logger;

    std::unordered_map<std::filesystem::path, std::shared_ptr<StoredType>> m_cache;
};

class ResourceManager
{
  public:
    ResourceManager(Logger &loggerRef) : m_textureCache(loggerRef)
    {
    }

    std::shared_ptr<TextureResource> fetch_texture(const std::filesystem::path &path)
    {
        return m_textureCache.fetch(path);
    }

    std::size_t purge_textures()
    {
        return m_textureCache.purge();
    }

    void clear()
    {
        m_textureCache.clear();
    }

  private:
    ResourceCache<TextureResource> m_textureCache;
};
} // namespace rotm
