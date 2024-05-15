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

constexpr static std::size_t BlockSizeElements = 0x100000;
constexpr static std::size_t BlockSizeBytes
    = BlockSizeElements * sizeof(std::int32_t);

auto Hash(const std::string& filepath) -> std::uint32_t {
  std::filesystem::path path{filepath};

  std::ifstream file(path, std::ios::binary | std::ios::in);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file '" + filepath + "'");
  }

  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(BlockSizeElements);
  while (!file.eof()) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    file.read(buffer, BlockSizeBytes);
    block.resize(file.gcount());

    if (!file.eof() && (file.bad() || file.fail())) {
      throw std::runtime_error(
          "failed during readings file '" + filepath + "'"
      );
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::smart
