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
  if (args.empty()) {
    throw std::invalid_argument("usage: filehash (filepath)+");
  }

  constexpr auto width = 8;
  const auto hash = filehash::silly::HashPar(args);
  std::cout << "0x" << std::hex << std::setfill('0') << std::setw(width) //
            << hash << '\n';
}

} // namespace filehash::app
