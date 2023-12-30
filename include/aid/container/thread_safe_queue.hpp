#pragma once

#include "aid/async/condition_variable.hpp"

#include <optional>
#include <queue>

namespace aid {
template <typename T> class thread_safe_queue {
public:
  void push(T value) {
    mQueue.with_lock([&](auto &queue) { queue.push(std::move(value)); });
    mQueue.notify_one();
  }

  std::optional<T> pop() {
    std::optional<T> ret;

    mQueue.with_lock([&](auto &queue) {
      if (!queue.empty()) {
        x.emplace(std::move(queue.front()));
        queue.pop();
      }
    });

    return ret;
  }

  void wait(std::stop_token token) {
    const auto hasElement = [](const auto &queue) { return !queue.empty(); };

    mQueue.wait(hasElement, token);
  }

private:
  condition_variable<std::queue<T>> mQueue;
};
} // namespace aid