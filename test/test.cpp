#include "app/process.hpp"
#include "app/smart.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

namespace filehash::test {

using filehash::smart::BlockSizeElements;

auto GenerateInput(std::default_random_engine& random)
    -> std::pair<std::stringstream, std::vector<std::uint32_t>> {

  constexpr std::size_t zero_freq = 10;

  static std::uniform_int_distribution<std::uint8_t> byte_dist{
      std::numeric_limits<std::uint8_t>::min(),
      std::numeric_limits<std::uint8_t>::max(),
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
    const std::uint8_t byte = is_zero ? 0 : byte_dist(random);
    stream << byte;
    buffer[j] = byte; // NOLINT
  }

  return std::make_pair(std::move(stream), std::move(block));
}

TEST_CASE("In-memory same as data_processor") {
  using filehash::smart::Hash;

  constexpr std::size_t seed = 1232142132;

  std::default_random_engine random{seed}; // NOLINT

  constexpr std::size_t rounds = 500;
  constexpr std::size_t batch = 5;
  for (std::size_t i = 0; i < rounds; ++i) {
    auto [stream, block] = GenerateInput(random);

    // FIXME: is this okay that, if p := process_block
    // p(concat(a, b, c)) == p(a) then p(b) then p(c)?
    const auto expected = data_processor_t{}.process_block(block);
    const auto actual = Hash(stream);
    REQUIRE(expected == actual);

    if (i % batch == 0) {
      WARN(
          "Testing iteration " << i << " with size " << block.size()
                               << " and hash " << actual << "..."
      );
    }
  }
}

} // namespace filehash::test
