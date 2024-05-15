#include "app/process.hpp"
#include "app/smart.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <random>
#include <sstream>
#include <vector>

namespace filehash::test {

TEST_CASE("In-memory same as data_processor") {
  using filehash::smart::BlockSizeElements;
  using filehash::smart::Hash;

  constexpr std::size_t seed = 1232142132;
  std::default_random_engine random{seed}; // NOLINT

  std::uniform_int_distribution<std::uint8_t> byte_dist{
      std::numeric_limits<std::uint8_t>::min(),
      std::numeric_limits<std::uint8_t>::max(),
  };

  std::uniform_int_distribution<std::size_t> size_dist{
      std::numeric_limits<std::size_t>::min(),
      BlockSizeElements / 100, // NOLINT
  };

  constexpr std::size_t rounds = 100;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    if (i % batch == 0) {
      INFO("Testing iteration " << i << "...");
    }

    const auto size = size_dist(random);

    std::stringstream stream;

    std::vector<std::uint32_t> block(size);
    auto* buffer = reinterpret_cast<std::uint8_t*>(block.data()); // NOLINT

    for (std::size_t j = 0; j < size; ++j) {
      const auto byte = byte_dist(random);
      stream << byte;
      buffer[j] = byte; // NOLINT
    }

    const auto expected = data_processor_t{}.process_block(block);
    const auto actual = Hash(stream);
    REQUIRE(expected == actual);
  }
}

} // namespace filehash::test
