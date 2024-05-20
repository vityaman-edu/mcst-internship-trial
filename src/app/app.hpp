#pragma once

#include "app/core.hpp"

#include <cstddef>
#include <vector>

namespace filehash::app {

struct AppConfig {
  enum class Method { SILLY, SMART };
  enum class ExecutionPolicy { SEQUENTIAL, CONCURRENT };

  Method method;
  ExecutionPolicy policy;
  std::size_t block_size_elements;
  std::vector<Path> paths;

  static auto Parse(int argc, char** argv) -> AppConfig;
};

auto FileHash(const AppConfig& config) -> HashCode;

auto Main(int argc, char** argv) -> int;

} // namespace filehash::app
