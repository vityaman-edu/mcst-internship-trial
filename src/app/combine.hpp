#pragma once

#include "app/core.hpp"
#include "app/defer.hpp"

#include <array>
#include <cstring>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace filehash::combine {

using Paths = std::vector<Path>;

auto HashCombine(const std::vector<HashCode>& hashes) -> HashCode;

template <class UnaryHash>
auto HashSeq(const Paths& files, UnaryHash hash) -> HashCode {
  std::vector<HashCode> hashes;
  hashes.reserve(files.size());
  for (const auto& file : files) {
    hashes.push_back(hash(file));
  }
  return HashCombine(hashes);
}

template <class UnaryHash>
auto HashPar(const Paths& files, UnaryHash hash) -> HashCode {
  // No memory fences used as I believe that writes must be
  // flushed somewhere in kernel at process exit.

  constexpr std::size_t CacheLineSize = 128;

  struct Result {
    HashCode hash;
    std::array<std::byte, CacheLineSize - sizeof(HashCode)> padding;
  };

  if (files.empty()) {
    return 0;
  }

  if (files.size() == 1) {
    return hash(files[0]);
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
        std::string("failed to map shared memory ") + strerror(errno)
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
        results[j].hash = hash(files[j]);
      }
      exit(0);
    }
  }

  std::vector<HashCode> hashes(files.size());
  for (std::size_t i = 0; i < files.size(); ++i) {
    hashes[i] = results[i].hash;
  }
  return HashCombine(hashes);
}

} // namespace filehash::combine
