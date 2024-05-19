#include "app/core.hpp"
#include "app/defer.hpp"
#include "app/process.hpp"
#include "app/silly.hpp"
#include "app/smart.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace filehash::test {

using filehash::BlockSizeElements;

auto GenerateInMemoryInput(
    std::default_random_engine& random,
    std::uint32_t max_blocks = 3,                // NOLINT
    std::uint32_t block_size = BlockSizeElements // NOLINT
) -> std::pair<std::stringstream, std::vector<std::uint32_t>> {

  std::uniform_int_distribution<char> byte_dist{
      std::numeric_limits<char>::min(),
      std::numeric_limits<char>::max(),
  };

  constexpr std::size_t zero_freq = 4;
  std::uniform_int_distribution<std::uint8_t> is_zero_dist{0, zero_freq};

  std::uniform_int_distribution<std::size_t> size_dist{
      0,
      static_cast<std::size_t>(max_blocks) * block_size,
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

auto GenerateFileInput(
    std::default_random_engine& random,
    const std::string& filepath = "text.txt" // NOLINT
) -> std::pair<std::string, std::vector<std::uint32_t>> {
  auto [stream, block] = GenerateInMemoryInput(random);

  const auto size = stream.str().size();

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

  return std::make_pair(filepath, std::move(block));
}

auto GenerateFilesInput(
    std::default_random_engine& random, const std::string& tag
) -> std::vector<std::string> {
  constexpr std::uint8_t MaxFiles = 16;

  std::uniform_int_distribution<std::uint8_t> count_dist{0, MaxFiles};

  const auto count = count_dist(random);
  std::vector<std::string> files;
  for (std::size_t i = 0; i < count; ++i) {
    auto [file, block]
        = GenerateFileInput(random, tag + "-" + std::to_string(i));
    files.emplace_back(std::move(file));
  }
  return files;
}

TEST_CASE("Data Processor Property") {
  std::uint32_t hash_whole = 0;
  {
    data_processor_t hasher;
    hash_whole = hasher.process_block({1, 2, 3, 4, 5, 0, 6, 7, 8, 9}); // NOLINT
  }

  std::uint32_t hash_chunked = 0;
  {
    data_processor_t hasher;
    hash_chunked = hasher.process_block({1, 2, 3});
    hash_chunked = hasher.process_block({4});
    hash_chunked = hasher.process_block({5, 0, 6}); // NOLINT
    hash_chunked = hasher.process_block({7, 8, 9}); // NOLINT
  };

  REQUIRE(hash_whole != hash_chunked);
}

TEST_CASE("In-memory") {
  constexpr std::size_t seed = 1232142132;
  std::default_random_engine random{seed}; // NOLINT

  constexpr std::size_t rounds = 25000;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    auto block_size
        = std::uniform_int_distribution<std::uint32_t>{8, 1024}(random);
    auto [stream, block] = GenerateInMemoryInput(random, 8, block_size);

    std::uint32_t expected = 0;
    data_processor_t hasher;
    for (std::size_t j = 0; j < block.size(); j += block_size) {
      auto chunk_size = std::min(
          static_cast<int>(block_size), //
          static_cast<int>(block.size() - j)
      );

      auto first = std::next(std::begin(block), static_cast<int>(j));
      auto last = std::next(first, chunk_size);
      expected = hasher.process_block({first, last});
    }

    const auto actual = filehash::smart::Hash(stream, block_size);

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

  constexpr std::size_t rounds = 100;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    auto [filepath, block] = GenerateFileInput(random);
    Defer defer([file = filepath] { std::filesystem::remove(file.c_str()); });

    const auto expected = data_processor_t{}.process_block(block);
    const auto silly = filehash::silly::Hash(filepath);
    const auto smart = filehash::smart::Hash(filepath);

    REQUIRE(smart == silly);
    REQUIRE(expected == silly);
    REQUIRE(expected == smart);

    if (i % batch == 0) {
      WARN(
          "Testing iteration " << i << " with size " << block.size()
                               << " and hash " << silly << "..."
      );
    }
  }
}

TEST_CASE("Fork-join") {
  constexpr std::size_t seed = 1232142132;
  std::default_random_engine random{seed}; // NOLINT

  constexpr std::size_t rounds = 25;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    const auto files = GenerateFilesInput(random, std::to_string(i));
    Defer removal([&] {
      for (const auto& file : files) {
        std::filesystem::remove(file.c_str());
      }
    });

    const auto seq = silly::HashSeq(files);
    const auto par = silly::HashPar(files);

    REQUIRE(seq == par);

    if (i % batch == 0) {
      WARN("Testing iteration " << i << " with count " << files.size());
    }
  }
}

} // namespace filehash::test
