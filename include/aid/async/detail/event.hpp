#pragma once

#include <atomic>

namespace aid::detail {
class event {
public:
  void set() {
    mFlag.test_and_set();
    mFlag.notify_all();
  }

  void wait() { mFlag.wait(false); }

private:
  std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
};
} // namespace aid::detail