#include "silly.hpp"
#include "core.hpp"
#include "defer.hpp"
#include "process.hpp"

#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

namespace filehash::silly {

auto ReadFull(int file, char* buffer, ssize_t size) -> ssize_t {
  ssize_t remaining = size;
  while (remaining != 0) {
    const auto count
        = read(file, buffer + size - remaining, remaining); // NOLINT
    if (count < 0) {
      return -1;
    }
    if (count == 0) {
      break;
    }
    remaining -= count;
  }
  return size - remaining;
}

auto Hash(const std::string& filepath) -> std::uint32_t {
  const auto file = open(filepath.c_str(), O_RDONLY | O_CLOEXEC); // NOLINT
  if (file < 0) {
    throw std::runtime_error("failed to open file '" + filepath + "'");
  }

  Defer defer([&] { close(file); });

  std::uint32_t hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(BlockSizeElements + 2);
  for (;;) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    const auto count = ReadFull(file, buffer, BlockSizeBytes);

    if (count < 0) {
      throw std::runtime_error("failed reading input");
    }
    if (count == 0) {
      break;
    }

    for (std::size_t i = 0; i < 8; ++i) { // NOLINT
      buffer[count + i] = 0;              // NOLINT
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::silly
