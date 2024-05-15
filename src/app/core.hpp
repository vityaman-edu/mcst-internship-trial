#pragma once

#include <cstdint>

namespace filehash {

constexpr std::size_t BlockSizeElements = 0x100000;

constexpr std::size_t BlockSizeBytes
    = BlockSizeElements * sizeof(std::uint32_t);

} // namespace filehash