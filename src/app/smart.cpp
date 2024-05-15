#include "smart.hpp"
#include "core.hpp"
#include "process.hpp"

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

  std::ifstream file(path, std::ios::binary | std::ios::in);
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
  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(BlockSizeElements + 2);
  while (!input.eof()) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    const auto count = ReadFull(input, buffer, BlockSizeBytes);
    for (std::size_t i = 0; i < 8; ++i) { // NOLINT
      buffer[count + i] = 0;              // NOLINT
    }

    if (!input.eof() && (input.bad() || input.fail())) {
      throw std::runtime_error("failed readings input");
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::smart
