module;

#include <atomic>

export module aid.async:manual_event;

export namespace aid {
class manual_event {
public:
  void set() noexcept {
    mFlag.test_and_set();
    mFlag.notify_all();
  }

  void wait() noexcept { mFlag.wait(false); }

  bool is_set() const noexcept { return mFlag.test(); }

private:
  std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
};
} // namespace aid