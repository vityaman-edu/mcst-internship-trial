#pragma once

#include <string>
#include <vector>

namespace filehash::app {

auto ParseArgs(int argc, char* argv[]) -> std::vector<std::string>; // NOLINT

auto Main(const std::vector<std::string>& args) -> void;

} // namespace filehash::app
