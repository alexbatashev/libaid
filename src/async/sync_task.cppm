// The sync_task class is heavily derived from Lewiss Baker's cppcoro.
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

#include <cassert>
#include <coroutine>
#include <exception>

export module aid.async:sync_task;

import :manual_event;

namespace aid {
export template <typename Value> class sync_task;

namespace detail {
template <typename Value> class sync_task_promise final {
  using coro_handle_t = std::coroutine_handle<sync_task_promise<Value>>;

public:
  using reference = Value &&;

  sync_task_promise() noexcept = default;

  void start(manual_event &event) {
    mEvent = &event;
    coro_handle_t::from_promise(*this).resume();
  }

  auto get_return_object() noexcept {
    return coro_handle_t::from_promise(*this);
  }

  std::suspend_always initial_suspend() const noexcept { return {}; }

  auto final_suspend() noexcept {
    struct notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coro_handle_t coro) const noexcept {
        coro.promise().mEvent->set();
      }

      void await_resume() noexcept {}
    };

    return notifier{};
  }

  auto yield_value(reference result) noexcept {
    mResult = std::addressof(result);
    return final_suspend();
  }

  void return_void() noexcept { assert(false); }

  void unhandled_exception() { mException = std::current_exception(); }

  reference result() {
    if (mException)
      std::rethrow_exception(mException);

    return static_cast<reference>(*mResult);
  }

private:
  manual_event *mEvent;
  std::remove_reference_t<Value> *mResult;
  std::exception_ptr mException;
};

template <> class sync_task_promise<void> {
  using coro_handle_t = std::coroutine_handle<sync_task_promise<void>>;

public:
  sync_task_promise() noexcept = default;

  void start(manual_event &event) {
    mEvent = &event;
    coro_handle_t::from_promise(*this).resume();
  }

  auto get_return_object() noexcept {
    return coro_handle_t::from_promise(*this);
  }

  std::suspend_always initial_suspend() const noexcept { return {}; }

  auto final_suspend() noexcept {
    struct notifier {
      bool await_ready() const noexcept { return false; }
      void await_suspend(coro_handle_t coro) const noexcept {
        coro.promise().mEvent->set();
      }

      void await_resume() noexcept {}
    };

    return notifier{};
  }

  void return_void() noexcept {}

  void unhandled_exception() { mException = std::current_exception(); }

  void result() {
    if (mException)
      std::rethrow_exception(mException);
  }

private:
  manual_event *mEvent;
  std::exception_ptr mException;
};
} // namespace detail

export template <typename Value> class sync_task final {
public:
  using promise_type = detail::sync_task_promise<Value>;
  using coro_handle_t = std::coroutine_handle<promise_type>;

  sync_task(coro_handle_t handle) noexcept : mHandle(handle) {}

  sync_task(sync_task &&t) noexcept : mHandle(t.mHandle) {
    t.mHandle = nullptr;
  }

  ~sync_task() {
    if (mHandle)
      mHandle.destroy();
  }

  sync_task(const sync_task &) = delete;
  sync_task &operator=(const sync_task &) = delete;

  void start(manual_event &event) noexcept { mHandle.promise().start(event); }

  decltype(auto) result() { return mHandle.promise().result(); }

private:
  coro_handle_t mHandle;
};
} // namespace aid
