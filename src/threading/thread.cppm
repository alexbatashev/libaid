module;

#define _LIBCPP_ENABLE_EXPERIMENTAL 1

#include <coroutine>
#include <thread>

export module aid.threading:thread;

import aid.containers;

namespace aid {
export class thread {
public:
  thread() {
    mThread = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
        if (mTasks.empty())
          mTasks.wait(token);

        if (token.stop_requested())
          break;

        auto task = mTasks.pop();

        if (task.has_value()) {
          task->resume();
        }
      }
    });
  }

  void enqueue(std::coroutine_handle<> task) { mTasks.push(std::move(task)); }

private:
  thread_safe_queue<std::coroutine_handle<>> mTasks;
  std::jthread mThread;
};
} // namespace aid