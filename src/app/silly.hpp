#pragma once

#include "core.hpp"

namespace filehash::silly {

auto Hash(const Path& path, std::size_t block_size) -> HashCode;

} // namespace filehash::silly
