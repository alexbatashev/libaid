// The task class is heavily derived from Lewiss Baker's cppcoro. The original
// copyright notice is below.
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

#pragma once

#include "aid/threading/sync_task.hpp"

#include <coroutine>
#include <variant>

namespace aid {
template <typename Value> class task;

namespace detail {
class task_promise_base {
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

template <typename Value> class task_promise final : public task_promise_base {
public:
  task_promise() noexcept = default;

  ~task_promise() {
    if (std::holds_alternative<Value>(mResult))
      std::get<Value>(mResult).~Value();
    else if (std::holds_alternative<std::exception_ptr>(mResult))
      std::get<std::exception_ptr>(mResult).~exception_ptr();
  }

  task<Value> get_return_object() noexcept;

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

template <> class task_promise<void> : public task_promise_base {
public:
  task_promise() noexcept = default;

  task<void> get_return_object() noexcept;

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

template <typename Value = void> class [[nodiscard]] task {
public:
  using promise_type = detail::task_promise<Value>;
  using value_type = Value;

  task() noexcept = default;
  task(const task &) = delete;

  task(task &&t) noexcept : mHandle{t.mHandle} { t.mHandle = nullptr; }

  explicit task(std::coroutine_handle<promise_type> handle) noexcept
      : mHandle{handle} {}

  task &operator=(task &&t) noexcept {
    if (std::addressof(t) != this) {
      if (mHandle)
        mHandle.destroy();

      mHandle = t.mHandle;
      t.mHandle = nullptr;
    }

    return *this;
  }

  ~task() {
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
inline task<Value> task_promise<Value>::get_return_object() noexcept {
  return aid::task<Value>{
      std::coroutine_handle<task_promise>::from_promise(*this)};
}

inline task<void> task_promise<void>::get_return_object() noexcept {
  return aid::task<void>{
      std::coroutine_handle<task_promise>::from_promise(*this)};
}

template <typename Value>
inline sync_task<std::remove_reference_t<Value>>
make_sync_task(task<Value> &&t) {
  co_yield co_await std::forward<task<Value>>(t);
}
} // namespace detail

template <typename Value> inline decltype(auto) sync_wait(task<Value> &&t) {
  detail::event evt;
  auto sync = detail::make_sync_task(std::forward<task<Value>>(t));
  sync.start(evt);
  evt.wait();

  return sync.result();
}
} // namespace aid