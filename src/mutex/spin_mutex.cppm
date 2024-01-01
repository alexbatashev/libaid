module;

#include <atomic>

export module aid.mutex:spin_mutex;

import aid.utils;

namespace aid {
/// spin_mutex is a synchronization primitive, that uses atomic variable and
/// causes thread trying acquire lock wait in loop while repeatedly check if
/// the lock is available.
export class spin_mutex {
public:
  void lock() {
    backoff b;
    while (mLock.test_and_set(std::memory_order_acquire))
      b.pause();
  }
  void unlock() { mLock.clear(std::memory_order_release); }

private:
  std::atomic_flag mLock = ATOMIC_FLAG_INIT;
};

/// shared_spin_mutex is a synchronization primitive, that allows RW-locks.
export class shared_spin_mutex {
public:
  static constexpr bool is_noexcept = true;

  void lock() noexcept {
    for (backoff b;; b.pause()) {
      uint32_t curState = mState.load(std::memory_order_relaxed);
      if (!(curState & BUSY)) {
        if (mState.compare_exchange_strong(curState, WRITER)) {
          break;
        }
        b.reset();
      } else if (!(curState & WRITER_PENDING)) {
        mState |= WRITER_PENDING;
      }
    }
  }

  void unlock() noexcept { mState &= READERS; }

  void lock_shared() noexcept {
    for (backoff b;; b.pause()) {
      uint32_t curState = mState.load(std::memory_order_relaxed);
      if (!(curState & (WRITER | WRITER_PENDING))) {
        uint32_t oldState = mState.fetch_add(ONE_READER);
        if (!(oldState & WRITER))
          break;

        mState -= ONE_READER;
      }
    }
  }

  void unlock_shared() noexcept {
    mState.fetch_sub(ONE_READER, std::memory_order_release);
  }

  void upgrade() noexcept {
    uint32_t curState = mState.load(std::memory_order_relaxed);
    if ((curState & READERS) == ONE_READER || !(curState & WRITER_PENDING)) {
      if (mState.compare_exchange_strong(curState,
                                         curState | WRITER | WRITER_PENDING)) {
        backoff b;
        while ((mState.load(std::memory_order_relaxed) & READERS) !=
               ONE_READER) {
          b.pause();
        }

        mState -= (ONE_READER + WRITER_PENDING);
        return;
      }
    }
    unlock_shared();
    lock();
  }

private:
  static constexpr uint32_t WRITER = 1 << 31;
  static constexpr uint32_t WRITER_PENDING = 1 << 30;
  static constexpr uint32_t READERS = ~(WRITER | WRITER_PENDING);
  static constexpr uint32_t ONE_READER = 1;
  static constexpr uint32_t BUSY = WRITER | READERS;
  std::atomic<uint32_t> mState{0};
};
} // namespace aid
