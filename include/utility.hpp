#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace rotm
{
/**
 * @brief Remove rotm namespace, return type, and parameters from a function name.
 * Designed with std::source_location::function_name in mind, which
 * should be the same as __PRETTY_FUNCTION__ in GCC.
 */
inline std::string_view clean_function_name(std::string_view funcName)
{
    // Position of argument list.
    const std::size_t paramStart = funcName.find('(');

    if (paramStart != std::string_view::npos)
    {
        // Position of the function's name.
        const std::size_t nameStart = funcName.rfind(' ', paramStart);

        // If the function has a return type, toss it.
        if (nameStart != std::string_view::npos)
            funcName.remove_prefix(nameStart + 1);

        // Remove the argument list.
        funcName = funcName.substr(0, paramStart - (nameStart == std::string_view::npos ? 0 : nameStart + 1));
    }

    // If function is inside rotm namespace, remove the specifier.
    // This should theoretically always be true, as code outside of
    // this codebase should never call rotm's logging functions.
    // If I decide to pipe raylib's log through a callback,
    // this could become useful.
    if (funcName.starts_with("rotm::"))
        funcName.remove_prefix(6);

    return funcName;
}

/**
 * @brief Creates a vector of bytes from a vector of bools.
 *
 */
inline std::vector<std::uint8_t> pack_bitset(const std::vector<bool> &bits)
{
    const std::size_t byteCount = bits.size() % 8 == 0 ? bits.size() / 8 : (bits.size() / 8) + 1;

    std::vector<std::uint8_t> out;
    out.reserve(byteCount);

    std::uint8_t bitCount = 0;
    std::uint8_t currentByte = 0;

    for (const auto &bit : bits)
    {
        if (bit)
            currentByte |= (0b00000001u << bitCount);

        ++bitCount;

        if (bitCount >= 8)
        {
            out.push_back(currentByte);

            bitCount = 0;
            currentByte = 0;
        }
    }

    if (bitCount > 0)
        out.push_back(currentByte);

    return out;
}

/**
 * @brief Convert a vector of bytes to a vector of bools.
 * NOTE: If bitset was not evenly divisible by 8 before packing,
 * caller is responsible for trimming it to the correct size!
 *
 * @param bytes Source vector.
 */
inline std::vector<bool> unpack_bitset(const std::vector<std::uint8_t> &bytes)
{
    using std::size_t;
    using std::uint8_t;

    std::vector<bool> out;
    out.reserve(bytes.size() * 8); // could be more than needed but it doesn't hurt anything

    for (const auto &byte : bytes)
    {
        for (uint8_t bitIdx = 0; bitIdx < 8; ++bitIdx)
        {
            const uint8_t testBit = 0b00000001 << bitIdx;

            out.push_back(byte & testBit);
        }
    }

    return out;
}

template <typename T> bool write_to_stream(std::ostream &file, const T &source, std::size_t *filePos = nullptr)
{
    static_assert(std::is_trivially_copyable_v<T>);

    file.write(reinterpret_cast<const char *>(&source), sizeof(T));

    if (file.good())
    {
        if (filePos)
            *filePos += sizeof(T);
        return true;
    }

    return false;
}

template <typename T> bool read_from_stream(std::istream &file, T &target, std::size_t *filePos = nullptr)
{
    static_assert(std::is_trivially_copyable_v<T>);

    file.read(reinterpret_cast<char *>(&target), sizeof(T));

    if (file.good())
    {
        if (filePos)
            *filePos += sizeof(T);
        return true;
    }

    return false;
}
} // namespace rotm
