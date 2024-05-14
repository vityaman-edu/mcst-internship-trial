#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace filehash::test {

auto factorial(uint32_t number) -> uint32_t {
  return number <= 1 ? number : factorial(number - 1) * number;
}

namespace {

TEST_CASE("Factorials are computed", "[factorial]") { // NOLINT
  REQUIRE(factorial(1) == 1);                         // NOLINT
  REQUIRE(factorial(2) == 2);                         // NOLINT
  REQUIRE(factorial(3) == 6);                         // NOLINT
  REQUIRE(factorial(10) == 3'628'800);                // NOLINT
}

} // namespace

} // namespace filehash::test