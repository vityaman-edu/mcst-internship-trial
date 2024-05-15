#pragma once

#include <cstdint>
#include <string>

namespace filehash::silly {

auto Hash(const std::string& filepath) -> std::uint32_t;

} // namespace filehash::silly
