module;

#include <atomic>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>

export module aid.async:future;

namespace aid {
export template <typename Value>
class future;

namespace detail {
class future_promise_base {
public:
  future_promise_base() noexcept = default;

  std::suspend_never initial_suspend() const noexcept { return {}; }

  std::suspend_never final_suspend() const noexcept { return {}; }

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

  void wait() const noexcept {
    if (!ready_.test())
      ready_.wait(false);
  }

  void notify_complete() noexcept {
    std::ignore = ready_.test_and_set();
    ready_.notify_all();
  }
protected:
  void rethrow_exception() {
    if (exception_ != nullptr)
      std::rethrow_exception(exception_);
  }

private:
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

  future<T> get_return_object() noexcept;

  void resume() noexcept {
    std::coroutine_handle<future_promise>::from_promise(*this).resume();
  }

  template <typename Value>
  requires std::convertible_to<Value&&, T>
  void return_value(Value &&value) noexcept(std::is_nothrow_constructible_v<T, Value&&>) {
    new (&data_[0]) T(std::forward<Value>(value));
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

export template <typename Value>
class [[nodiscard]] future {
public:
  using promise_type = detail::future_promise<Value>;
  using value_type = Value;

  future() noexcept = default;

  explicit future(std::coroutine_handle<promise_type> handle) noexcept
      : handle_(handle) {}

  future(const future &other) noexcept : handle_(other.handle_) {
    if (handle_)
      handle_.retain();
  }

  future(future &&other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  future &operator=(const future &other) noexcept {
    if (handle_ != other.handle_) {
      destroy();

      handle_ = other.handle_;

      if (handle_)
        handle_.retain();
    }

    return *this;
  }

  future &operator=(future &&other) noexcept {
    if (&other != this) {
      destroy();

      handle_ = other.handle_;
      other.handle_ = nullptr;
    }

    return *this;
  }

  ~future() { destroy(); }

  void wait_sync() noexcept {
    if (handle_)
      handle_.promise().wait();
  }

  auto operator co_await() const noexcept { return awaitable{handle_}; }

  decltype(auto) get_result() {
    if (!handle_)
      throw std::runtime_error("broken future");

    return handle_.promise().result();
  }

private:
  template <typename T>
  friend bool operator==(const future<T> &, const future<T> &) noexcept;

  struct awaitable {
    awaitable(std::coroutine_handle<promise_type> handle) noexcept
        : handle_(handle) {}

    bool await_ready() const noexcept {
      return !handle_ || handle_.promise().is_ready();
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiter) noexcept {
      handle_.promise().wait();
      return awaiter;
    }

    decltype(auto) await_ready() {
      if (!handle_)
        throw std::runtime_error("broken coroutine");

      return handle_.promise().result();
    }
    std::coroutine_handle<promise_type> handle_;
  };

  void destroy() noexcept {
    if (handle_ && handle_.promise().release()) {
      // FIXME: why does this crash???
      // handle_.destroy();
      handle_ = nullptr;
    }
  }

  std::coroutine_handle<promise_type> handle_ = nullptr;
};

template <typename T>
bool operator==(const future<T> &lhs, const future<T> &rhs) noexcept {
  return lhs.handle_ == rhs.handle_;
}

template <typename T>
bool operator!=(const future<T> &lhs, const future<T> &rhs) noexcept {
  return !(lhs == rhs);
}

namespace detail {
template <typename Value>
future<Value> future_promise<Value>::get_return_object() noexcept {
  return future<Value>{
      std::coroutine_handle<future_promise<Value>>::from_promise(*this)};
}
} // namespace detail
}