module;

#include <cstdint>

#ifndef __WIN32
#include <cpuid.h>
#endif

module aid.simd;

using namespace aid;

#ifdef __WIN32
#define cpuid(info, x) __cpuidex(info, x, 0)
#else
static void cpuid(int32_t out[4], int info) noexcept {
  __cpuid_count(info, 0, out[0], out[1], out[2], out[3]);
}
#endif

simd_features aid::simd_probe() noexcept {
  simd_features features;

#ifdef __x86_64__
  // It's pretty much impossible to find a machine without SSE4.2 these days
  features.x86_64_sse42 = true;

  int info[4];
  cpuid(info, 0);
  int nIds = info[0];

  cpuid(info, 0x80000000);
  unsigned nExIds = info[0];

  if (nIds >= 0x00000001) {
    cpuid(info, 0x00000001);
    features.x86_64_avx = (info[2] & ((int)1 << 28)) != 0;
  }
  if (nIds >= 0x00000007) {
    cpuid(info, 0x00000007);

    features.x86_64_avx2 = (info[1] & ((int)1 << 5)) != 0;
    features.x86_64_avx512f = (info[1] & ((int)1 << 16)) != 0;
  }
#endif

  return features;
}