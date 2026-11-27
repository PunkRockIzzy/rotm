#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace rotm
{
struct Vec2I
{
    int x, y;

    inline constexpr auto operator<=>(const Vec2I& other) const noexcept = default;
};
} // namespace rotm

namespace std
{
template <> struct hash<rotm::Vec2I>
{
    static constexpr size_t operator()(const rotm::Vec2I& other) noexcept
    {
        // in case of platform fuckery
        static_assert(sizeof(size_t) == 8);
        static_assert(sizeof(int) == 4);

        size_t out = 0;

        out |= static_cast<uint32_t>(other.x);
        out <<= 32;
        out |= static_cast<uint32_t>(other.y);

        return out;
    }
};
} // namespace std
