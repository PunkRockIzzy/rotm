#pragma once

#include <filesystem>

namespace rotm
{
template <typename StoredType> class Resource
{
  public:
    Resource(const std::filesystem::path &path) : m_path(path)
    {
    }

    virtual ~Resource() = default;

    virtual const StoredType &get() const
    {
        return m_data;
    }

  protected:
    StoredType m_data;
    std::filesystem::path m_path;
};
} // namespace rotm
