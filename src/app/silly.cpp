#include "silly.hpp"
#include "core.hpp"
#include "defer.hpp"
#include "process.hpp"

#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

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

auto Hash(const Path& path, std::size_t block_size) -> HashCode {
  const ssize_t block_size_bytes
      = static_cast<std::int64_t>(block_size)
        * static_cast<std::int64_t>(sizeof(std::uint32_t));

  const auto file = open(path.c_str(), O_RDONLY | O_CLOEXEC); // NOLINT
  if (file < 0) {
    throw std::runtime_error("failed to open file '" + path.string() + "'");
  }

  Defer defer([&] { close(file); });

  HashCode hash = 0;
  data_processor_t processor;

  std::vector<std::uint32_t> block(block_size + 2);
  for (;;) {
    auto* buffer = reinterpret_cast<char*>(block.data()); // NOLINT
    const auto count = ReadFull(file, buffer, block_size_bytes);

    if (count < 0) {
      throw std::runtime_error("failed reading input");
    }
    if (count == 0) {
      break;
    }

    if (count != block_size_bytes) {
      for (std::size_t j = count; j < block_size * sizeof(std::uint32_t); ++j) {
        buffer[j] = 0;
      }
      block.resize(
          count / sizeof(std::uint32_t)
          + (count % sizeof(std::uint32_t) == 0 ? 0 : 1)
      );
    }

    hash = processor.process_block(block);
  }

  return hash;
}

} // namespace filehash::silly
