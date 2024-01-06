module;

export module aid.simd:probe;

export namespace aid {
struct simd_features {
  bool x86_64_sse42 = false;
  bool x86_64_avx = false;
  bool x86_64_avx2 = false;
  bool x86_64_avx512f = false;
};

simd_features simd_probe() noexcept;
} // namespace aid