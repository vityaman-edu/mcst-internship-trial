#include "app/smart.hpp"
#include "app/core.hpp"
#include "app/process.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

namespace filehash::smart {

auto ReadFull(std::istream& input, char* buffer, std::streamsize size)
    -> std::size_t {
  std::streamsize remaining = size;
  while (remaining != 0) {
    input.read(buffer + size - remaining, remaining); // NOLINT
    const auto count = input.gcount();
    if (count == 0) {
      break;
    }
    remaining -= count;
  }
  return size - remaining;
}

auto Hash(std::istream& input, std::size_t block_size) -> HashCode {
  if (block_size == 0) {
    throw std::invalid_argument("block size must be positive");
  }

  const std::size_t block_size_bytes = block_size * sizeof(std::uint32_t);
  const auto block_ssize_bytes = static_cast<std::streamsize>(block_size_bytes);

  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(block_size);
  while (!input.eof()) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    const auto count = ReadFull(input, buffer, block_ssize_bytes);

    if (!input.eof() && (input.bad() || input.fail())) {
      throw std::runtime_error("failed readings input");
    }

    block.resize(count / sizeof(std::uint32_t));
    hash = processor.process_block(block);
  }

  return hash;
}

auto Hash(const Path& path, std::size_t block_size) -> HashCode {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file '" + path.string() + "'");
  }
  return Hash(file, block_size);
}

} // namespace filehash::smart
