#pragma once

#include "aid/async/exclusive.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace aid {
template <typename T>
class condition_variable : public exclusive<T, std::mutex> {
public:
  explicit condition_variable(T &&value) : exclusive(std::forward(value)) {}

  void notify_one() { mCV.notify_one(); }
  void notify_all() { mCV.notify_all(); }

  template <typename F> void wait(F &&f) {
    std::unique_lock lock{mMutex};
    mCV.wait(lock,
             [&, this]() { return std::invoke(std::forward(f), mValue); });
  }

  template <typename F> void wait(F &&f, std::stop_token token) {
    std::unique_lock lock{mMutex};
    mCV.wait(lock, token,
             [&, this]() { return std::invoke(std::forward(f), mValue); });
  }

private:
  std::condition_variable_any mCV;
};
} // namespace aid