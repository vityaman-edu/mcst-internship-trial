#pragma once

#include "core.hpp"

#include <cstddef>

namespace filehash::smart {

auto Hash(const Path& path, std::size_t block_size) -> HashCode;

} // namespace filehash::smart
