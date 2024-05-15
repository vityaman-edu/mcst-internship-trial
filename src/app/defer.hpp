#pragma once

#include <utility>
namespace filehash {

template <class Action>
class Defer {
public:
  Defer(const Defer&) = delete;
  Defer(Defer&&) = delete;
  auto operator=(const Defer&) -> Defer& = delete;
  auto operator=(Defer&&) -> Defer& = delete;

  explicit Defer(Action action) : action_(std::move(action)) {}

  ~Defer() { std::move(action_)(); }

private:
  Action action_;
};

} // namespace filehash