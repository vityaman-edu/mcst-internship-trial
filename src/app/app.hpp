#pragma once

#include "core.hpp"

#include <string>
#include <vector>

namespace filehash::app {

struct AppConfig {
  enum class Method { SILLY, SMART };

  Method method;
  std::vector<std::string> files;

  static auto Parse(int argc, char** argv) -> AppConfig;
};

auto FileHash(const AppConfig& config) -> HashCode;

auto Main(int argc, char** argv) -> int;

} // namespace filehash::app
