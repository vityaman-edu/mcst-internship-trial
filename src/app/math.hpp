#pragma once

namespace filehash {

template <class T, class U>
auto DivCeil(T value, U divider) {
  return (value == 0) ? 0 : (1 + ((value - 1) / divider));
}

} // namespace filehash
