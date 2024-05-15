#include "app/app.hpp"

#include <exception>
#include <iostream>

auto main(int argc, char* argv[]) -> int try {
  const auto args = filehash::app::ParseArgs(argc, argv);
  filehash::app::Main(args);
  return 0;
} catch (const std::exception& exception) {
  std::cerr << "error: " << exception.what() << '\n';
  return 1;
}
