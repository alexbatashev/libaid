#pragma once

#if defined(__has_include) && __has_include(<immintrin.h>) && defined(__x86_64__)
#define HAS_PAUSE
#include <immintrin.h>
#endif

#include <thread>

namespace aid::detail {
class backoff {
public:
  void pause() {
    if (mCount <= LOOPS_BEFORE_YIELD) {
#ifdef HAS_PAUSE
      _mm_pause();
#elif __ARM_ARCH_7A__ || __aarch64__
      __asm__ __volatile__("yield" ::: "memory");
#else
      std::this_thread::yield();
#endif
      mCount *= 2;
    } else {
      std::this_thread::yield();
    }
  }

  void reset() noexcept { mCount = 1; }

private:
  static constexpr uint32_t LOOPS_BEFORE_YIELD = 16;
  uint32_t mCount = 1;
};
} // namespace aid::detail
