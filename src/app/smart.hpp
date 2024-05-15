#pragma once

#include <cstdint>
#include <string>

namespace filehash::smart {

constexpr std::size_t BlockSizeElements = 0x100000;

constexpr std::size_t BlockSizeBytes
    = BlockSizeElements * sizeof(std::uint32_t);

auto Hash(const std::string& filepath) -> std::uint32_t;

auto Hash(std::istream& input) -> std::uint32_t;

} // namespace filehash::smart
