module;

#include <condition_variable>
#include <mutex>
#include <thread>

export module aid.mutex:condition_variable;

import :exclusive;

namespace aid {
export template <typename T>
class condition_variable : public exclusive<T, std::mutex> {
private:
  using super = exclusive<T, std::mutex>;

public:
  explicit condition_variable(T &&value) : super(std::forward<T>(value)) {}

  void notify_one() { mCV.notify_one(); }
  void notify_all() { mCV.notify_all(); }

  template <typename F> void wait(F &&f) {
    std::unique_lock lock{super::mMutex};
    mCV.wait(lock, [&, this]() {
      return std::invoke(std::forward<F>(f), super::mValue);
    });
  }

  template <typename F> void wait(F &&f, std::stop_token token) {
    std::unique_lock lock{super::mMutex};
    mCV.wait(lock, token, [&, this]() {
      return std::invoke(std::forward<F>(f), super::mValue);
    });
  }

private:
  std::condition_variable_any mCV;
};
} // namespace aid