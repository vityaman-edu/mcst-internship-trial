#pragma once

#include "app/core.hpp"

#include <cstddef>

namespace filehash::silly {

auto Hash(const Path& path, std::size_t block_size) -> HashCode;

} // namespace filehash::silly
