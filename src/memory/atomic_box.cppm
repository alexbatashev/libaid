module;

#include <atomic>

export module aid.memory:atomic_box;

export namespace aid {
template <typename T>
class atomic_box {
public:
  atomic_box(T *ptr) : ptr_(ptr) {}

  T *load(std::memory_order order) const noexcept { return ptr_.load(order); }

  T *exchange(T *ptr, std::memory_order order) noexcept {
    return ptr_.exchange(ptr, order);
  }

  ~atomic_box() { delete ptr_.load(); }

private:
  std::atomic<T *> ptr_;
};
} // namespace aid