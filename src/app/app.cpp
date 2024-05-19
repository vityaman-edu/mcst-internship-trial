#include "app.hpp"
#include "silly.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
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
  auto files = ParseArgs(argc, argv);
  files.erase(std::begin(files));

  if (files.empty()) {
    throw std::invalid_argument("expected at least one filepath");
  }

  return {
      .method = AppConfig::Method::SILLY,
      .files = files,
  };
}

auto FileHash(const AppConfig& config) -> HashCode {
  switch (config.method) {
  case AppConfig::Method::SILLY:
    return filehash::silly::HashPar(config.files);
  case AppConfig::Method::SMART:
    throw std::invalid_argument("not implemented");
  }
  throw std::runtime_error("unexpected method");
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
