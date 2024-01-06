module;

#include <atomic>
#include <concepts>
#include <type_traits>
#include <exception>
#include <coroutine>
#include <cstdint>

export module aid.async:future;

namespace aid {
namespace detail {
class future_promise_base {
public:
  future_promise_base() noexcept = default;

  std::suspend_never initial_suspend() const noexcept { return {}; } 

  void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

  bool is_ready() const noexcept {
    return ready_.test();
  }

  bool has_exception() const noexcept {
    return exception_ != nullptr;
  }

  void retain() noexcept {
    ref_count_.fetch_add(1, std::memory_order_relaxed);
  }

  // Returns true if coroutine is ready to be released.
  bool release() noexcept {
    return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1;
  }

  void wait() noexcept {
    ready_.wait(false);
  }

  void notify_complete() noexcept {
    std::ignore = ready_.test_and_set();
    ready_.notify_all();
  }
protected:
  void rethrow_exception() {
    std::rethrow_exception(exception_);
  }

private:
  friend struct final_awaiter;

  struct final_awaiter {
    bool await_ready() noexcept {
      // ...
    }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> handle) noexcept {

    }

    void await_resume() {}
  };

  std::atomic<std::uint32_t> ref_count_ = 1;
  std::exception_ptr exception_ = nullptr;
  std::atomic_flag ready_ = ATOMIC_FLAG_INIT;
};

template <typename T>
class future_promise : public future_promise_base {
  using super = future_promise_base;
public:
  future_promise() noexcept = default;

  ~future_promise() {
    if (super::is_ready() && !super::has_exception()) {
      std::destroy_at(std::launder(reinterpret_cast<T*>(&data_[0])));
    }
  }

  template <typename Value>
  requires std::convertible_to<Value&&, T>
  void return_value(Value &&value) noexcept(std::is_nothrow_constructible_v<T, Value&&>) {
    new (&data[0]) T(std::forward<Value>(value));
    super::notify_complete();
  }

  T &result() {
    super::rethrow_exception();
    return *std::launder(reinterpret_cast<T*>(&data_[0]));
  }
private:
  alignas(T) char data_[sizeof(T)];
};
};

export template <typename T>
class [[nodiscard]] future {
public:
  using promise_type = detail::future_promise<T>;
  using value_type = Value;

  void wait_sync() noexcept {
    handle_.promise().wait();
  }
private:
  std::coroutine_handle<promise_type> handle_;
};
}