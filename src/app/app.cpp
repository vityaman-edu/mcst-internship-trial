#include "app.hpp"
#include "silly.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace filehash::app {

auto ParseArgs(int argc, char* argv[]) -> std::vector<std::string> { // NOLINT
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]); // NOLINT
  }
  return args;
}

auto Main(const std::vector<std::string>& args) -> void {
  constexpr auto width = 8;

  if (args.size() != 1) {
    throw std::invalid_argument("usage: filehash <filepath>");
  }

  const auto& filepath = args[0];
  const auto hash = filehash::silly::Hash(filepath);
  std::cout << "0x" << std::hex << std::setfill('0') << std::setw(width) //
            << hash << '\n';
}

} // namespace filehash::app
