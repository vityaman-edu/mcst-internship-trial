#include "app.hpp"
#include "combine.hpp"
#include "silly.hpp"
#include "smart.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace filehash::app {

constexpr auto* RESET = "\033[0m";
constexpr auto* RED = "\033[31m";
constexpr auto* BLUE = "\033[34m";
constexpr auto* GREEN = "\033[32m";

auto ParseArgs(int argc, char** argv) -> std::vector<std::string> {
  std::vector<std::string> args;
  args.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return args;
}

auto AppConfig::Parse(int argc, char** argv) -> AppConfig {
  constexpr auto DEFAULT_METHOD = AppConfig::Method::SILLY;
  constexpr auto DEFAULT_POLICY = AppConfig::ExecutionPolicy::CONCURRENT;
  constexpr auto DEFAULT_BLOCK_SIZE = 0x100000;

  auto args = ParseArgs(argc, argv);

  std::vector<Path> paths;
  paths.reserve(args.size() - 1);
  for (std::size_t i = 1; i < args.size(); ++i) {
    paths.emplace_back(args[i]);
  }

  if (paths.empty()) {
    throw std::invalid_argument("expected at least one filepath");
  }

  return {
      .method = DEFAULT_METHOD,
      .policy = DEFAULT_POLICY,
      .block_size_elements = DEFAULT_BLOCK_SIZE,
      .paths = paths,
  };
}

auto FileHash(const AppConfig& config) -> HashCode {
  switch (config.method) {
  case AppConfig::Method::SILLY: {
    const auto hash = [&](const auto& path) {
      return silly::Hash(path, config.block_size_elements);
    };

    switch (config.policy) {
    case AppConfig::ExecutionPolicy::SEQUENTIAL:
      return combine::HashSeq(config.paths, hash);
    case AppConfig::ExecutionPolicy::CONCURRENT:
      return combine::HashPar(config.paths, hash);
    };

  } break;

  case AppConfig::Method::SMART: {
    const auto hash = [&](const auto& path) {
      return smart::Hash(path, config.block_size_elements);
    };

    switch (config.policy) {
    case AppConfig::ExecutionPolicy::SEQUENTIAL:
      return combine::HashSeq(config.paths, hash);
    case AppConfig::ExecutionPolicy::CONCURRENT:
      return combine::HashPar(config.paths, hash);
    };

  } break;
  };

  throw std::runtime_error("unexpected method or execution policy");
}

auto Main(int argc, char** argv) -> int try {
  constexpr auto width = 2 * sizeof(std::uint32_t);

  const auto hash = FileHash(AppConfig::Parse(argc, argv));

  std::cout << "0x" << std::hex << std::setfill('0') << std::setw(width);
  std::cout << hash << '\n' << std::flush;

  return 0;
} catch (const std::invalid_argument& exception) {
  std::cerr << RED << "error: " << RESET << exception.what() << '\n';
  std::cerr << std::flush;

  std::cerr << BLUE << "usage: " << RESET;
  std::cerr << GREEN << "filehash" << RESET << " <filepath>+" << '\n';

  return 1;
} catch (const std::exception& exception) {
  std::cerr << RED << "error: " << RESET << exception.what() << '\n';
  std::cerr << std::flush;

  return 2;
}

} // namespace filehash::app
