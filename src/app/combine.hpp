#pragma once

#include "app/core.hpp"
#include "app/defer.hpp"

#include <cstring>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace filehash::combine {

using Paths = std::vector<Path>;

template <class Iter>
auto HashCombine(Iter begin, Iter end) -> HashCode {
  HashCode combined = 0;
  for (auto it = begin; it != end; ++it) {
    combined ^= *it;
  }
  return combined;
}

template <class UnaryHash>
auto HashSeq(const Paths& files, UnaryHash hash) -> HashCode {
  std::vector<HashCode> hashes;
  hashes.reserve(files.size());
  for (const auto& file : files) {
    hashes.push_back(hash(file));
  }
  return HashCombine(std::begin(hashes), std::end(hashes));
}

template <class UnaryHash>
auto HashPar(const Paths& files, UnaryHash hash) -> HashCode {
  // No memory fences used as I believe that writes must be
  // flushed somewhere in kernel at process exit.

  if (files.empty()) {
    return 0;
  }

  if (files.size() == 1) {
    return hash(files[0]);
  }

  const auto shared_memory_size_bytes = sizeof(HashCode) * files.size();

  auto* hashes = static_cast<HashCode*>(mmap(
      /* addr   = */ nullptr,
      /* length = */ shared_memory_size_bytes,
      /* prot   = */ PROT_READ | PROT_WRITE,
      /* flags  = */ MAP_SHARED | MAP_ANONYMOUS,
      /* fd     = */ -1,
      /* offset = */ 0
  ));

  if (hashes == MAP_FAILED) {
    throw std::runtime_error(
        std::string("failed to map shared memory ") + strerror(errno)
    );
  }

  Defer unmap([&] {
    munmap(hashes, shared_memory_size_bytes);
    hashes = nullptr;
  });

  for (std::size_t i = 0; i < files.size(); ++i) {
    hashes[i] = 0;
  }

  {
    std::vector<pid_t> children;
    children.reserve(files.size());

    Defer wait_chidren([&] {
      for (const auto child : children) {
        int stat = 0;
        waitpid(child, &stat, 0);
      }
    });

    for (std::size_t i = 0; i < files.size(); ++i) {
      const pid_t pid = fork();

      if (pid == -1) {
        throw std::runtime_error("fork failed");
      }

      if (pid != 0) {
        children.push_back(pid);
        continue;
      }

      hashes[i] = hash(files[i]);
      exit(0);
    }
  }

  return HashCombine(hashes, hashes + files.size());
}

} // namespace filehash::combine
