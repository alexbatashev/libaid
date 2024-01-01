module;

#include <functional>
#include <mutex>

export module aid.mutex:exclusive;

import :lock;

namespace aid {
export template <typename Value, lockable Impl> class exclusive {
public:
  explicit exclusive(Value &&value) : mValue(std::forward<Value>(value)) {}

  template <typename F> decltype(auto) with_lock(F &&f) {
    std::unique_lock _{mMutex};
    return std::invoke(std::forward<F>(f), mValue);
  }

  template <typename F> decltype(auto) with_lock(F &&f) const {
    std::unique_lock _{mMutex};
    return std::invoke(std::forward<F>(f), mValue);
  }

protected:
  mutable Impl mMutex;
  Value mValue;
};
} // namespace aid