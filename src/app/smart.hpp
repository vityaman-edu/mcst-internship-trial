#pragma once

#include <cstdint>
#include <string>

namespace filehash::smart {

auto Hash(const std::string& filepath) -> std::uint32_t;

auto Hash(std::istream& input) -> std::uint32_t;

auto Hash(std::istream& input, std::uint32_t block_size) -> std::uint32_t;

} // namespace filehash::smart
