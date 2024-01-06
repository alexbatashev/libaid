module;

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <utility>

export module aid.simd:dispatch;

import :probe;

namespace aid::experimental {
export template <unsigned MaxBitWidth, bool HasSSE42, bool HasAVX, bool HasAVX2,
                 bool HasAVX512F>
struct simd_tag {
  static constexpr unsigned max_simd_register_bit_width = MaxBitWidth;
  static constexpr bool has_sse42 = HasSSE42;
  static constexpr bool has_avx = HasAVX;
  static constexpr bool has_avx2 = HasAVX2;
  static constexpr bool has_avx512f = HasAVX512F;
};

template <typename F, typename... Args>
__attribute__((target("sse4.2"))) auto dispatch_sse42(Args &&...args) {
  F func;
  simd_tag</*max_simd_register_bit_width=*/128,
           /*has_sse42=*/true,
           /*has_avx=*/false,
           /*has_avx2=*/false,
           /*has_avx512f=*/false>
      tag;
  return func(tag, std::forward<Args>(args)...);
}

template <typename F, typename... Args>
__attribute__((target("avx"))) auto dispatch_avx(Args &&...args) {
  F func;
  simd_tag</*max_simd_register_bit_width=*/256,
           /*has_sse42=*/true,
           /*has_avx=*/true,
           /*has_avx2=*/false,
           /*has_avx512f=*/false>
      tag;
  return func(tag, std::forward<Args>(args)...);
}

template <typename F, typename... Args>
__attribute__((target("avx2"))) auto dispatch_avx2(Args &&...args) {
  F func;
  simd_tag</*max_simd_register_bit_width=*/256,
           /*has_sse42=*/true,
           /*has_avx=*/true,
           /*has_avx2=*/true,
           /*has_avx512f=*/false>
      tag;
  return func(tag, std::forward<Args>(args)...);
}

template <typename F, typename... Args>
__attribute__((target("avx512f"))) auto dispatch_avx512f(Args &&...args) {
  F func;
  simd_tag</*max_simd_register_bit_width=*/256,
           /*has_sse42=*/true,
           /*has_avx=*/true,
           /*has_avx2=*/true,
           /*has_avx512f=*/true>
      tag;
  return func(tag, std::forward<Args>(args)...);
}

export template <typename F, typename... Args>
__attribute__((always_inline)) auto dispatch(F &&f, Args &&...args) {
  auto features = simd_probe();
  if (features.x86_64_avx512f) {
    return dispatch_avx512f<std::remove_reference_t<F>>(
        std::forward<Args>(args)...);
  } else if (features.x86_64_avx2) {
    return dispatch_avx2<std::remove_reference_t<F>>(
        std::forward<Args>(args)...);
  } else if (features.x86_64_avx) {
    return dispatch_avx<std::remove_reference_t<F>>(
        std::forward<Args>(args)...);
  }
  // Pretty much every CPU today supports SSE 4.2, no need for a fallback
  return dispatch_sse42<std::remove_reference_t<F>>(
      std::forward<Args>(args)...);
}
} // namespace aid::experimental