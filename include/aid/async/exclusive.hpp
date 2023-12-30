#pragma once

#include "aid/async/lock.hpp"

#include <mutex>

namespace aid {
template <typename Value, lockable Impl> class exclusive {
public:
  explicit exclusive(Value &&value) : mValue(std::forward(value)) {}

  template <typename T> decltype(auto) with_lock(F &&f) {
    std::unique_lock _{mMutex};
    return std::invoke(std::forward(f), mValue);
  }

  template <typename T> decltype(auto) with_lock(F &&f) const {
    std::unique_lock _{mMutex};
    return std::invoke(std::forward(f), mValue);
  }

protected:
  mutable Impl mMutex;
  Value mValue;
};
} // namespace aid