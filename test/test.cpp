#include "app/core.hpp"
#include "app/process.hpp"
#include "app/silly.hpp"
#include "app/smart.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <limits>
#include <ostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

namespace filehash::test {

using filehash::BlockSizeElements;

auto GenerateInMemoryInput(std::default_random_engine& random)
    -> std::pair<std::stringstream, std::vector<std::uint32_t>> {

  constexpr std::size_t zero_freq = 10;

  static std::uniform_int_distribution<char> byte_dist{
      std::numeric_limits<char>::min(),
      std::numeric_limits<char>::max(),
  };

  static std::uniform_int_distribution<std::uint8_t> is_zero_dist{0, zero_freq};

  static std::uniform_int_distribution<std::size_t> size_dist{
      0, 3 * BlockSizeElements
  };

  const auto size = size_dist(random);

  std::stringstream stream;

  std::vector<std::uint32_t> block(size);
  auto* buffer = reinterpret_cast<std::uint8_t*>(block.data()); // NOLINT

  for (std::size_t j = 0; j < size; ++j) {
    const bool is_zero = is_zero_dist(random) > 0;
    const char byte = is_zero ? '\0' : byte_dist(random);
    stream << byte;
    buffer[j] = byte; // NOLINT
  }

  return std::make_pair(std::move(stream), std::move(block));
}

auto GenerateFileInput(std::default_random_engine& random)
    -> std::pair<std::string, std::vector<std::uint32_t>> {
  auto [stream, block] = GenerateInMemoryInput(random);

  const auto size = stream.str().size();

  std::string filepath = "test.txt";
  std::ofstream file(filepath);
  file.exceptions(std::ofstream::badbit | std::ofstream::failbit);

  std::size_t index = 0;
  for (const auto part : block) {
    char buffer[4] = {0};                              // NOLINT
    *reinterpret_cast<std::uint32_t*>(&buffer) = part; // NOLINT
    for (std::size_t j = 0; index < size && j < 4; ++j, ++index) {
      file << buffer[j]; // NOLINT
    }
  }

  return std::make_pair(std::move(filepath), std::move(block));
}

TEST_CASE("In-memory") {
  constexpr std::size_t seed = 1232142132;
  std::default_random_engine random{seed}; // NOLINT

  constexpr std::size_t rounds = 500;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    auto [stream, block] = GenerateInMemoryInput(random);

    // FIXME: is this okay that, if p := process_block
    // p(concat(a, b, c)) == p(a) then p(b) then p(c)?
    const auto expected = data_processor_t{}.process_block(block);
    const auto actual = filehash::smart::Hash(stream);
    REQUIRE(expected == actual);

    if (i % batch == 0) {
      WARN(
          "Testing iteration " << i << " with size " << block.size()
                               << " and hash " << actual << "..."
      );
    }
  }
}

TEST_CASE("Out-memory") {
  constexpr std::size_t seed = 1232142132;
  std::default_random_engine random{seed}; // NOLINT

  constexpr std::size_t rounds = 250;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    auto [filepath, block] = GenerateFileInput(random);

    const auto expected = data_processor_t{}.process_block(block);
    const auto silly = filehash::silly::Hash(filepath);
    const auto smart = filehash::smart::Hash(filepath);

    REQUIRE(smart == silly);
    REQUIRE(expected == silly);
    REQUIRE(expected == smart);

    std::filesystem::remove(filepath.c_str());

    if (i % batch == 0) {
      WARN(
          "Testing iteration " << i << " with size " << block.size()
                               << " and hash " << silly << "..."
      );
    }
  }
}

} // namespace filehash::test
