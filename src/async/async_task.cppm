// The async_task class is heavily derived from Lewiss Baker's cppcoro.
// The original copyright notice is below.
//
// Copyright 2017 Lewis Baker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

module;

#include <coroutine>
#include <variant>

export module aid.async:async_task;

import :sync_task;
import :manual_event;

namespace aid {
export template <typename Value> class async_task;

namespace detail {
class async_task_promise_base {
public:
  std::suspend_always initial_suspend() noexcept { return {}; }
  auto final_suspend() noexcept { return awaitable{}; }

  void set_continuation(std::coroutine_handle<> cont) { mContinuation = cont; }

private:
  struct awaitable {
    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<Promise> handle) noexcept {
      return handle.promise().mContinuation;
    }

    void await_resume() noexcept {}
  };

  std::coroutine_handle<> mContinuation;
};

template <typename Value> class async_task_promise final : public async_task_promise_base {
public:
  async_task_promise() noexcept = default;

  ~async_task_promise() {
    if (std::holds_alternative<Value>(mResult))
      std::get<Value>(mResult).~Value();
    else if (std::holds_alternative<std::exception_ptr>(mResult))
      std::get<std::exception_ptr>(mResult).~exception_ptr();
  }

  async_task<Value> get_return_object() noexcept;

  void unhandled_exception() noexcept { mResult = std::current_exception(); }

  template <typename RetValue>
    requires std::is_convertible_v<RetValue &&, Value>
  void return_value(RetValue &&value) noexcept(
      std::is_nothrow_constructible_v<Value, RetValue &&>) {
    mResult = std::forward<RetValue>(value);
  }

  Value &result() {
    if (std::holds_alternative<std::exception_ptr>(mResult))
      std::rethrow_exception(std::get<std::exception_ptr>(mResult));

    return std::get<Value>(mResult);
  }

private:
  std::variant<std::monostate, Value, std::exception_ptr> mResult;
};

template <> class async_task_promise<void> : public async_task_promise_base {
public:
  async_task_promise() noexcept = default;

  async_task<void> get_return_object() noexcept;

  void return_void() noexcept {}

  void unhandled_exception() noexcept { mException = std::current_exception(); }

  void result() {
    if (mException)
      std::rethrow_exception(mException);
  }

private:
  std::exception_ptr mException;
};
} // namespace detail

export template <typename Value = void> class [[nodiscard]] async_task {
public:
  using promise_type = detail::async_task_promise<Value>;
  using value_type = Value;

  async_task() noexcept = default;
  async_task(const async_task &) = delete;

  async_task(async_task &&t) noexcept : mHandle{t.mHandle} { t.mHandle = nullptr; }

  explicit async_task(std::coroutine_handle<promise_type> handle) noexcept
      : mHandle{handle} {}

  async_task &operator=(async_task &&t) noexcept {
    if (std::addressof(t) != this) {
      if (mHandle)
        mHandle.destroy();

      mHandle = t.mHandle;
      t.mHandle = nullptr;
    }

    return *this;
  }

  ~async_task() {
    if (mHandle)
      mHandle.destroy();
  }

  bool is_ready() const noexcept { return !mHandle || mHandle.done(); }

  auto operator co_await() const & noexcept {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;

      decltype(auto) await_resume() {
        if (!this->mHandle)
          std::abort();

        return this->mHandle.promise().result();
      }
    };

    return awaitable{mHandle};
  }

  auto operator co_await() const && noexcept {
    struct awaitable : awaitable_base {
      using awaitable_base::awaitable_base;

      decltype(auto) await_resume() {
        if (!this->mHandle)
          std::abort();

        return std::move(this->mHandle.promise().result());
      }
    };

    return awaitable{mHandle};
  }

private:
  struct awaitable_base {
    awaitable_base(std::coroutine_handle<promise_type> handle) noexcept
        : mHandle(handle) {}

    bool await_ready() const noexcept { return !mHandle || mHandle.done(); }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> awaiting) noexcept {
      mHandle.promise().set_continuation(awaiting);
      return mHandle;
    }

    std::coroutine_handle<promise_type> mHandle = nullptr;
  };

  std::coroutine_handle<promise_type> mHandle = nullptr;
};

namespace detail {
template <typename Value>
inline async_task<Value> async_task_promise<Value>::get_return_object() noexcept {
  return aid::async_task<Value>{
      std::coroutine_handle<async_task_promise>::from_promise(*this)};
}

inline async_task<void> async_task_promise<void>::get_return_object() noexcept {
  return aid::async_task<void>{
      std::coroutine_handle<async_task_promise>::from_promise(*this)};
}

template <typename Value>
inline sync_task<std::remove_reference_t<Value>>
make_sync_task(async_task<Value> &&t) {
  co_yield co_await std::forward<async_task<Value>>(t);
}
} // namespace detail

export template <typename Value> inline decltype(auto) sync_wait(async_task<Value> &&t) {
  manual_event evt;
  auto sync = detail::make_sync_task(std::forward<async_task<Value>>(t));
  sync.start(evt);
  evt.wait();

  return sync.result();
}
} // namespace aid