module;

#include <list>
#include <optional>
#include <stop_token>

export module aid.containers:thread_safe_queue;

import aid.mutex;
import :vector;

namespace aid {
/// Multi-producer multi-consumer thread safe queue implementation.
export template <typename T>
class thread_safe_queue {
public:
  thread_safe_queue() : queue_(vector<T>()){};
  thread_safe_queue(size_t capacity) : queue_(vector<T>()) {
    queue_.with_lock([capacity](auto &queue) { queue.reserve(capacity); });
  };

  /// @brief Adds an element to the end of the queue.
  /// @param value is a value to be placed on the queue.
  void push(T value) {
    queue_.with_lock([&](auto &queue) {
      queue.push_back(std::move(value));
      bottom_++;
    });
    queue_.notify_one();
  }

  /// @brief Extracts an element from the beginning of the queue.
  /// @return std::nullopt if queue is empty, an element otherwise.
  std::optional<T> pop() {
    std::optional<T> ret;

    queue_.with_lock([&](auto &queue) {
      if (top_ != bottom_) {
        ret.emplace(std::move(queue[top_++]));
        if (top_ == bottom_) {
          top_ = 0;
          bottom_ = 0;
          queue.clear();
        } else {
          defragment(queue);
        }
      }
    });

    return ret;
  }

  /// @brief Extracts an element from the beginning of the queue if lock is not
  /// owned by anyone.
  /// @return std::nullopt if retrieval failed, an element otherwise.
  std::optional<T> try_pop() {
    std::optional<T> ret;

    queue_.try_with_lock([&](auto &queue) {
      if (top_ != bottom_) {
        ret.emplace(std::move(queue[top_++]));
        if (top_ == bottom_) {
          top_ = 0;
          bottom_ = 0;
          queue.clear();
        } else {
          defragment(queue);
        }
      }
    });

    return ret;
  }

  bool empty() const noexcept { return top_ == bottom_; }

  /// @brief Blocking wait until an element is enqueued.
  /// @param token is a cancellation token.
  void wait(std::stop_token token) {
    const auto had_element = [this](const auto &queue) {
      return top_ != bottom_;
    };

    queue_.wait(had_element, token);
  }

private:
  void defragment(vector<T> &queue) {
    if (top_ < queue.capacity() / 3)
      return;

    for (size_t i = top_; i < bottom_; i++)
      queue[i - top_] = std::move(queue[i]);

    queue.erase(&queue[bottom_ - top_]);
    top_ = 0;
    bottom_ = queue.size();
  }

  condition_variable<vector<T>> queue_;
  size_t top_ = 0;
  size_t bottom_ = 0;
};
} // namespace aid