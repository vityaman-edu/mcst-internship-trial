#include "smart.hpp"
#include "core.hpp"
#include "process.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

namespace filehash::smart {

auto Hash(const std::string& filepath) -> std::uint32_t {
  std::filesystem::path path{filepath};

  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file '" + filepath + "'");
  }

  return Hash(file);
}

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

auto Hash(std::istream& input) -> std::uint32_t {
  return Hash(input, BlockSizeElements);
}

auto Hash(std::istream& input, std::uint32_t block_size) -> std::uint32_t {
  assert(block_size > 4);

  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(block_size);
  while (!input.eof()) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    const auto count
        = ReadFull(input, buffer, block_size * sizeof(std::uint32_t));
    if (count != block_size * sizeof(std::uint32_t)) {
      for (std::size_t j = count; j < block_size * sizeof(std::uint32_t); ++j) {
        buffer[j] = 0;
      }
      block.resize(
          count / sizeof(std::uint32_t)
          + (count % sizeof(std::uint32_t) == 0 ? 0 : 1)
      );
    }

    if (!input.eof() && (input.bad() || input.fail())) {
      throw std::runtime_error("failed readings input");
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::smart
