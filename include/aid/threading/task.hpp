#pragma once

#include <coroutine>

namespace aid {
class task {
public:
  task() = default;

  task(const task &) = delete;
  task(task &&t) : mHandle{t.mHandle} { t.mHandle = nullptr; }

  template <typename... Ts>
  task(std::coroutine_handle<Ts...> handle) : mHandle{handle} {}

  void resume() {
    mHandle();
    mHandle = nullptr;
  }

private:
  std::coroutine_handle<> mHandle;
};
} // namespace aid