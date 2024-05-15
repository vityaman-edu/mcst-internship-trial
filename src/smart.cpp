#include "smart.hpp"
#include "process.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

namespace filehash::smart {

auto Hash(std::istream& input) -> std::uint32_t;

auto Hash(const std::string& filepath) -> std::uint32_t {
  std::filesystem::path path{filepath};

  std::ifstream file(path, std::ios::binary | std::ios::in);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file '" + filepath + "'");
  }

  return Hash(file);
}

auto Hash(std::istream& input) -> std::uint32_t {
  constexpr static std::size_t BlockSizeElements = 0x100000;
  constexpr static std::size_t BlockSizeBytes
      = BlockSizeElements * sizeof(std::uint32_t);

  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(BlockSizeElements);
  while (!input.eof()) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    input.read(buffer, BlockSizeBytes);
    block.resize(input.gcount());

    if (!input.eof() && (input.bad() || input.fail())) {
      throw std::runtime_error("failed during readings input");
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::smart
