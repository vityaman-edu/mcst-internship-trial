#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace filehash::silly {

auto Hash(const std::string& filepath) -> std::uint32_t;

auto HashSeq(const std::vector<std::string>& files) -> std::uint32_t;

auto HashPar(const std::vector<std::string>& files) -> std::uint32_t;

} // namespace filehash::silly
