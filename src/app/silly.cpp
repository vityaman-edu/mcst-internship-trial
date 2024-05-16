#include "silly.hpp"
#include "core.hpp"
#include "defer.hpp"
#include "process.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
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

auto HashSeq(const std::vector<std::string>& files) -> std::uint32_t {
  std::uint32_t hash = 0;
  for (const auto& file : files) {
    hash ^= Hash(file);
  }
  return hash;
}

/// No memory fences used as I believe that writes must be
/// flushed somewhere in kernel at process exit.
auto HashPar(const std::vector<std::string>& files) -> std::uint32_t {
  constexpr std::size_t CacheLineSize = 128;

  struct Result { // NOLINT
    std::uint32_t hash;
    std::array<char, CacheLineSize - sizeof(std::uint32_t)> padding;
  };

  if (files.empty()) {
    return 0;
  }

  const auto shared_memory_size_bytes = sizeof(Result) * files.size();

  auto* results = static_cast<Result*>(mmap(
      /* addr   = */ nullptr,
      /* length = */ shared_memory_size_bytes,
      /* prot   = */ PROT_READ | PROT_WRITE,
      /* flags  = */ MAP_SHARED | MAP_ANONYMOUS,
      /* fd     = */ -1,
      /* offset = */ 0
  ));

  if (results == MAP_FAILED) {
    throw std::runtime_error(
        std::string("failed to map shared memory ") + strerror(errno) // NOLINT
    );
  }

  Defer unmap([&] {
    munmap(results, shared_memory_size_bytes);
    results = nullptr;
  });

  for (std::size_t i = 0; i < files.size(); ++i) {
    results[i].hash = 0; // NOLINT
  }

  {
    const auto parallelism = std::thread::hardware_concurrency();

    std::vector<pid_t> children;
    children.reserve(parallelism);

    Defer wait_chidren([&] {
      for (const auto child : children) {
        int stat = 0;
        waitpid(child, &stat, 0);
      }
    });

    for (std::size_t i = 0; i < parallelism && i < files.size(); ++i) {
      const pid_t pid = fork();

      if (pid == -1) {
        throw std::runtime_error("fork failed");
      }

      if (pid != 0) {
        children.push_back(pid);
        continue;
      }

      for (std::size_t j = i; j < files.size(); j += parallelism) {
        results[j].hash = Hash(files[j]); // NOLINT
      }
      exit(0); // NOLINT
    }
  }

  std::uint32_t hash = 0;
  for (std::size_t i = 0; i < files.size(); ++i) {
    hash ^= results[i].hash; // NOLINT
  }
  return hash;
}

} // namespace filehash::silly
