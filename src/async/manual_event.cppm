module;

#include <atomic>

export module aid.async:manual_event;

export namespace aid {
class manual_event {
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