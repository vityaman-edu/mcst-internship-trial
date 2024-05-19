#include "app/app.hpp"
#include "app/defer.hpp"
#include "app/math.hpp"
#include "app/process.hpp"
#include "app/silly.hpp"
#include "app/smart.hpp"
#include "catch2/catch_message.hpp"
#include "catch2/generators/catch_generators_range.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <ostream>
#include <random>
#include <string>
#include <vector>

namespace filehash::test {

using Random = std::default_random_engine;

struct FileContentConfig {
  std::size_t zero_frequency;
  std::size_t min_size;
  std::size_t max_size;
};

auto GenerateFileContent(Random& random, const FileContentConfig& config)
    -> std::string {

  std::uniform_int_distribution<char> byte_dist{
      std::numeric_limits<char>::min(), std::numeric_limits<char>::max()
  };

  std::uniform_int_distribution<std::size_t> is_zero_dist{
      0, config.zero_frequency
  };

  std::uniform_int_distribution<std::size_t> size_dist{
      config.min_size, config.max_size
  };

  std::string content;
  const auto size = size_dist(random);
  for (std::size_t j = 0; j < size; ++j) {
    const bool is_zero = is_zero_dist(random) > 0;
    const char byte = is_zero ? '\0' : byte_dist(random);
    content.push_back(byte);
  }
  return content;
}

auto ToBlock(const std::string& content) {
  std::vector<std::uint32_t> block;
  block.resize(DivCeil(content.size(), sizeof(std::uint32_t)));

  char* data = reinterpret_cast<char*>(block.data()); // NOLINT
  for (std::size_t i = 0; i < content.size(); ++i) {
    data[i] = content[i];
  }

  return block;
}

auto Write(const std::string& content, const Path& path) -> void {
  std::ofstream file(path);
  file.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  for (const auto byte : content) {
    file << byte;
  }
}

auto ExpectedHash(const std::string& content, std::size_t block_size) {
  std::uint32_t expected = 0;

  const auto block = ToBlock(content);

  data_processor_t hasher;
  for (std::size_t i = 0; i < block.size(); i += block_size) {
    auto chunk_size = static_cast<int>(std::min(block_size, block.size() - i));

    auto first = std::next(std::begin(block), static_cast<int>(i));
    auto last = std::next(first, chunk_size);
    expected = hasher.process_block({first, last});
  }

  return expected;
}

auto GenerateInputFiles(
    Random& random,
    FileContentConfig config,
    std::size_t count,
    const std::string& prefix
) {
  std::vector<std::filesystem::path> paths;
  for (std::size_t i = 0; i < count; ++i) {
    auto filepath = prefix + "-" + std::to_string(i);
    Write(GenerateFileContent(random, config), filepath);
    paths.emplace_back(std::move(filepath));
  }
  return paths;
}

TEST_CASE("Data Processor Property") {
  std::uint32_t hash_whole = 0;
  {
    data_processor_t hasher;
    hash_whole = hasher.process_block({1, 2, 3, 4, 5, 0, 6, 7, 8, 9});
  }

  std::uint32_t hash_chunked = 0;
  {
    data_processor_t hasher;
    hash_chunked = hasher.process_block({1, 2, 3});
    hash_chunked = hasher.process_block({4});
    hash_chunked = hasher.process_block({5, 0, 6});
    hash_chunked = hasher.process_block({7, 8, 9});
  };

  REQUIRE(hash_whole != hash_chunked);
}

TEST_CASE("DivCeil") {
  REQUIRE(DivCeil(0, 4) == 0);
  REQUIRE(DivCeil(1, 4) == 1);
  REQUIRE(DivCeil(2, 4) == 1);
  REQUIRE(DivCeil(4, 4) == 1);
  REQUIRE(DivCeil(5, 4) == 2);
  REQUIRE(DivCeil(8, 4) == 2);
}

TEST_CASE("Block Splitting") {
  constexpr std::size_t seed = 1'232'142'132;
  constexpr std::size_t rounds = 250;
  constexpr auto filepath = "/tmp/test";

  std::default_random_engine random{seed}; // NOLINT

  static const std::vector<FileContentConfig> configs = {
      {.zero_frequency = 2, .min_size = 0, .max_size = 0   },
      {.zero_frequency = 2, .min_size = 1, .max_size = 1   },
      {.zero_frequency = 2, .min_size = 2, .max_size = 2   },
      {.zero_frequency = 2, .min_size = 3, .max_size = 3   },
      {.zero_frequency = 2, .min_size = 4, .max_size = 4   },
      {.zero_frequency = 2, .min_size = 5, .max_size = 5   },
      {.zero_frequency = 2, .min_size = 0, .max_size = 2048},
      {.zero_frequency = 2, .min_size = 0, .max_size = 4096},
      {.zero_frequency = 6, .min_size = 0, .max_size = 8192},
  };

  for (const auto& config : configs) {
    WARN("Testing with max size " << config.max_size << '\n');

    std::uniform_int_distribution<std::uint32_t> block_size_dist{1, 1024};

    for (std::size_t i = 0; i < rounds; ++i) {
      const auto content = GenerateFileContent(random, config);

      Write(content, filepath);

      const auto block_size = block_size_dist(random);

      const auto expected = ExpectedHash(content, block_size);
      const auto silly = filehash::silly::Hash(filepath, block_size);
      const auto smart = filehash::smart::Hash(filepath, block_size);

      REQUIRE(expected == smart);
      REQUIRE(expected == silly);
    }

    std::filesystem::remove(filepath);
  }
}

TEST_CASE("Application") {
  constexpr std::size_t seed = 1'232'142'132;
  constexpr std::size_t rounds = 64;
  constexpr std::size_t batch = 4;
  constexpr auto prefix = "/tmp/test";

  std::default_random_engine random{seed}; // NOLINT

  FileContentConfig config{
      .zero_frequency = 4,
      .min_size = 0,
      .max_size = 4096,
  };

  std::uniform_int_distribution<std::uint32_t> block_size_dist{1, 1024};
  std::uniform_int_distribution<std::uint32_t> files_count_dist{1, 12};

  for (std::size_t i = 0; i < rounds; ++i) {
    if (i % batch == 0) {
      std::cout << "Testing application " << i << "..." << '\n';
    }

    const auto block_size = block_size_dist(random);
    const auto files_count = files_count_dist(random);

    const auto paths = GenerateInputFiles(random, config, files_count, prefix);
    Defer remove([&] {
      for (const auto& path : paths) {
        std::filesystem::remove(path);
      }
    });

    const auto silly_seq = app::FileHash({
        .method = app::AppConfig::Method::SILLY,
        .policy = app::AppConfig::ExecutionPolicy::SEQUENTIAL,
        .block_size_elements = block_size,
        .paths = paths,
    });

    const auto smart_seq = app::FileHash({
        .method = app::AppConfig::Method::SMART,
        .policy = app::AppConfig::ExecutionPolicy::SEQUENTIAL,
        .block_size_elements = block_size,
        .paths = paths,
    });

    const auto silly_par = app::FileHash({
        .method = app::AppConfig::Method::SILLY,
        .policy = app::AppConfig::ExecutionPolicy::CONCURRENT,
        .block_size_elements = block_size,
        .paths = paths,
    });

    const auto smart_par = app::FileHash({
        .method = app::AppConfig::Method::SMART,
        .policy = app::AppConfig::ExecutionPolicy::CONCURRENT,
        .block_size_elements = block_size,
        .paths = paths,
    });

    REQUIRE(silly_seq == silly_par);
    REQUIRE(silly_seq == smart_seq);
    REQUIRE(silly_par == smart_par);
  }
}

TEST_CASE("Large file") {
  constexpr std::size_t seed = 1'232'142'132;
  constexpr std::size_t rounds = 10;
  constexpr auto filepath = "/tmp/test";
  constexpr auto block_size = 0x100000;

  std::default_random_engine random{seed}; // NOLINT

  static const std::vector<FileContentConfig> configs = {
      {.zero_frequency = 4, .min_size = 0, .max_size = 1UL * block_size},
      {.zero_frequency = 4, .min_size = 0, .max_size = 2UL * block_size},
      {.zero_frequency = 4, .min_size = 0, .max_size = 3UL * block_size},
      {.zero_frequency = 5, .min_size = 0, .max_size = 4UL * block_size},
  };

  for (const auto& config : configs) {
    WARN(
        "Testing with "
        << "zero frequency " << config.zero_frequency //
        << ",  max size " << config.max_size << '\n'
    );

    for (std::size_t i = 0; i < rounds; ++i) {
      const auto content = GenerateFileContent(random, config);
      Write(content, filepath);

      const auto expected = ExpectedHash(content, block_size);
      const auto actual = app::FileHash({
          .method = app::AppConfig::Method::SILLY,
          .policy = app::AppConfig::ExecutionPolicy::CONCURRENT,
          .block_size_elements = block_size,
          .paths = {filepath},
      });

      REQUIRE(expected == actual);
    }

    std::filesystem::remove(filepath);
  }
}

} // namespace filehash::test
