module;

#include <atomic>
#include <concepts>
#include <cstddef>
#include <list>
#include <optional>

export module aid.containers:lock_free_deque;

import :vector;
import :fixed_array;

import aid.memory;

namespace aid {
/// @brief An implementation of a lock-free deque (a.k.a. Chase-Lev deque).
/// @tparam T element type
///
/// The following papers can be used as a reference:
/// - Dynamic Circular Work-Stealing Deque
///   url: https://www.dre.vanderbilt.edu/~schmidt/PDF/work-stealing-dequeue.pdf
/// - Correct and Efficient Work-Stealing for Weak Memory Models
///   url: https://inria.hal.science/hal-00802885/document
export template <typename T>
  requires std::default_initializable<T>
class lock_free_deque {
public:
  static_assert(std::is_trivially_copyable_v<T>);

  explicit lock_free_deque(std::size_t size)
      : data_(new fixed_array<T>(size)) {}

  void push_back(T &&item) {
    std::size_t bottom = bottom_.load(std::memory_order_relaxed);
    std::size_t top = top_.load(std::memory_order_acquire);

    fixed_array<T> *data = get_or_grow(top, bottom);

    (*data)[bottom % data->size()] = std::forward<T>(item);

    std::atomic_thread_fence(std::memory_order_release);

    bottom_.store(bottom + 1, std::memory_order_relaxed);
  }

  std::optional<T> pop_back() {
    std::size_t bottom = bottom_.load(std::memory_order_relaxed) - 1;
    fixed_array<T> *data = data_.load(std::memory_order_relaxed);
    bottom_.store(bottom, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);

    std::size_t top = top_.load(std::memory_order_acquire);

    if (top <= bottom) {
      T *obj = &(*data)[bottom % data->size()];
      if (top == bottom) {
        if (!top_.compare_exchange_strong(top,
                                          top + 1,
                                          std::memory_order_seq_cst,
                                          std::memory_order_relaxed))
          obj = nullptr;
        bottom_.store(bottom + 1, std::memory_order_relaxed);
      }

      if (obj)
        return *obj;

      return std::nullopt;
    }

    bottom_.store(bottom + 1, std::memory_order_relaxed);
    return std::nullopt;
  }

  std::optional<T> pop_front() {
    std::size_t top = top_.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    std::size_t bottom = bottom_.load(std::memory_order_acquire);

    T *obj = nullptr;
    if (top < bottom) {
      fixed_array<T> *data = data_.load(std::memory_order_relaxed);
      obj = &(*data)[top % data->size()];
      if (!top_.compare_exchange_strong(top,
                                        top + 1,
                                        std::memory_order_seq_cst,
                                        std::memory_order_relaxed))
        obj = nullptr;
    }

    if (obj)
      return *obj;
    return std::nullopt;
  }

  bool empty() const noexcept {
    std::size_t bottom = bottom_.load(std::memory_order_relaxed);
    std::size_t top = top_.load(std::memory_order_acquire);

    return bottom <= top;
  }

private:
  fixed_array<T> *get_or_grow(std::size_t top, std::size_t bottom) {
    auto *data = data_.load(std::memory_order_relaxed);
    if (distance(bottom, top) == data->size()) {
      auto new_data = new fixed_array<T>(data->size() + data->size() / 2);
      auto old_ptr = data_.exchange(new_data, std::memory_order_relaxed);
      for (std::size_t i = top; i < bottom; i++)
        (*new_data)[i % new_data->size()] = (*old_ptr)[i % old_ptr->size()];

      // FIXME: this is not thread safe
      prev_.emplace_back(old_ptr);
      return new_data;
    }

    return data_.load(std::memory_order_relaxed);
  }

  constexpr std::size_t distance(std::size_t a, std::size_t b) const noexcept {
    return a > b ? a - b : b - a;
  }

  // FIXME implement array shrink

  std::atomic<std::size_t> top_ = 0;
  std::atomic<std::size_t> bottom_ = 0;
  atomic_box<fixed_array<T>> data_;

  std::list<std::unique_ptr<fixed_array<T>>> prev_;
};
} // namespace aid