module;

#include <cassert>
#include <coroutine>
#include <thread>
#include <vector>

export module aid.threading;

import aid.mutex;

import :thread;

export namespace aid {
class thread_pool {
public:
  thread_pool() : mThreads(std::thread::hardware_concurrency()) {}
  thread_pool(size_t numThreads) : mThreads(numThreads) {}
  thread_pool(const thread_pool &) = delete;
  thread_pool &operator=(const thread_pool &) = delete;

  size_t get_num_threads() const { return mThreads.size(); }

  auto schedule() { return awaiter{this}; }

private:
  friend struct awaiter;

  struct awaiter {
    explicit awaiter(thread_pool *pool) noexcept : mPool(pool) {}

    constexpr bool await_ready() const noexcept { return false; }
    constexpr void await_resume() const noexcept {}

    void await_suspend(std::coroutine_handle<> handle) {
      assert(mPool);
      mPool->enqueue(handle);
    }

  private:
    thread_pool *mPool;
  };

  void enqueue(std::coroutine_handle<> handle) {
    std::unique_lock _{mMutex};
    mThreads[mThreadIdx].enqueue(handle);
    mThreadIdx++;
    if (mThreadIdx == mThreads.size())
      mThreadIdx = 0;
  }

  spin_mutex mMutex;
  size_t mThreadIdx = 0;
  std::vector<thread> mThreads;
};
} // namespace aid