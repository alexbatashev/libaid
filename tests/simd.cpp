#include <catch2/catch_test_macros.hpp>

#include <iostream>

import aid.simd;

TEST_CASE("Basic arithmetics", "[simd]") {
  aid::simd<int, 2> a{1, 1};
  aid::simd<int, 2> b{2, 3};

  auto c = a + b;

  REQUIRE(c[0] == 3);
  REQUIRE(c[1] == 4);
}

TEST_CASE("Experimental dispatch", "[simd][experimental]") {

  unsigned width =
      aid::experimental::dispatch([](auto tag) __attribute__((always_inline)) {
        using tag_t = decltype(tag);
        return tag_t::max_simd_register_bit_width;
      });

  unsigned real_width = [] {
    auto features = aid::simd_probe();
    if (features.x86_64_avx512f)
      return 512;
    else if (features.x86_64_avx2 || features.x86_64_avx)
      return 256;
    else if (features.x86_64_sse42)
      return 128;
    return 0;
  }();

  REQUIRE(width == real_width);
}