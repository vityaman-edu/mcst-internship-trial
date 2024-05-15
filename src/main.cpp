#include <iomanip>
#include <iostream>

#include "app/smart.hpp"

auto main(int /*argc*/, char* /*argv*/[]) -> int {
  constexpr auto width = 8;
  const auto* filepath = ".clangd";
  std::cout << "0x" << std::hex << std::setfill('0') << std::setw(width)
            << filehash::smart::Hash(filepath) << '\n';
  return 0;
}
