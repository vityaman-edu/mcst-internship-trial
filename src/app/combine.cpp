#include "combine.hpp"

namespace filehash::combine {

auto HashCombine(const std::vector<HashCode>& hashes) -> HashCode {
  HashCode combined = 0;
  for (const auto hash : hashes) {
    combined ^= hash;
  }
  return combined;
}

} // namespace filehash::combine
