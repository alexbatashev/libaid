#pragma once

#include "aid/async/condition_variable.hpp"

#include <list>
#include <optional>

namespace aid {
template <typename T> class thread_safe_queue {
public:
  thread_safe_queue() : mQueue(std::list<T>()) {}
  void push(T value) {
    mQueue.with_lock([&](auto &queue) { queue.push_back(std::move(value)); });
    mQueue.notify_one();
  }

  std::optional<T> pop() {
    std::optional<T> ret;

    mQueue.with_lock([&](auto &queue) {
      if (!queue.empty()) {
        ret.emplace(std::move(queue.front()));
        queue.pop_front();
      }
    });

    return ret;
  }

  bool empty() {
    bool isEmpty = true;

    mQueue.with_lock([&](auto &queue) { isEmpty = queue.empty(); });

    return isEmpty;
  }

  template <typename F> std::optional<T> find_if(F &&pred) {
    std::optional<T> ret;

    mQueue.with_lock([&](auto &queue) {
      for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (pred(*it)) {
          ret.emplace(std::move(*it));
          queue.erase(it);
          break;
        }
      }
    });

    return ret;
  }

  void wait(std::stop_token token) {
    const auto hasElement = [](const auto &queue) { return !queue.empty(); };

    mQueue.wait(hasElement, token);
  }

private:
  condition_variable<std::list<T>> mQueue;
};
} // namespace aid