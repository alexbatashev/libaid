module;

#include <cassert>

export module aid.simd:types;

export namespace aid {

// X86 register widths
#if defined(__AVX512F__)
inline constexpr unsigned max_simd_register_bit_width = 512;
#elif defined(__AVX__)
inline constexpr unsigned max_simd_register_bit_width = 256;
#elif defined(__SSE__)
inline constexpr unsigned max_simd_register_bit_width = 128;
#endif

template <typename T,
          unsigned Dim1 = max_simd_register_bit_width / (8 * sizeof(T)),
          unsigned Dim2 = 1>
class simd {
  static_assert(false, "Unsupported SIMD configuration");
};

template <typename T, unsigned Dim1>
class simd<T, Dim1, 1> {
  static_assert(Dim1 * sizeof(T) <= max_simd_register_bit_width,
                "Unsupported SIMD configuration");

public:
#ifdef __clang__
  using value_type = T __attribute__((ext_vector_type(Dim1)));
#endif

  constexpr simd() noexcept : value_(0) {}

  template <typename... Ts>
  simd(Ts... ts) noexcept : value_(value_type{ts...}) {}

  explicit constexpr simd(value_type value) : value_(value) {}

  constexpr T operator[](unsigned idx) const noexcept {
    assert(idx < Dim1);
    return value_[idx];
  }

  constexpr void set(unsigned idx, T value) noexcept { value_[idx] = value; }

  template <unsigned Idx>
  constexpr T get() const noexcept {
    static_assert(Idx < Dim1, "Out-of-index access");
    return value_[Idx];
  }

  template <unsigned Idx>
  constexpr void set(T value) noexcept {
    static_assert(Idx < Dim1, "Out-of-index access");
    value_[Idx] = value;
  }

  constexpr simd operator+(const simd &rhs) const noexcept {
    return simd{value_ + rhs.value_};
  }

  constexpr simd operator-(const simd &rhs) const noexcept {
    return simd{value_ - rhs.value_};
  }

  constexpr simd operator*(const simd &rhs) const noexcept {
    return simd{value_ * rhs.value_};
  }

  constexpr simd operator/(const simd &rhs) const noexcept {
    return simd{value_ * rhs.value_};
  }

  constexpr simd<bool, Dim1> operator==(const simd &rhs) const noexcept {
    return simd<bool, Dim1>{value_ == rhs.value_};
  }

  constexpr T dot(const simd &rhs) const noexcept {
#ifdef __clang__
    return __builtin_reduce_add(value_ * rhs.value_);
#endif
  }

private:
  friend simd abs(simd);
  friend simd ceil(simd);
  friend simd floor(simd);
  friend simd sin(simd);
  friend simd cos(simd);
  friend simd log(simd);
  friend simd log2(simd);
  friend simd log10(simd);
  friend simd exp(simd);
  friend simd exp2(simd);
  friend simd sqrt(simd);
  friend simd pow(simd, simd);
  friend simd max(simd, simd);
  friend simd min(simd, simd);
  friend simd fma(simd, simd, simd);
  value_type value_;
};

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> abs(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_abs(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> ceil(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_ceil(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> floor(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_floor(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> sin(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_sin(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> cos(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_cos(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> log(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_log(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> log2(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_log2(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> log10(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_log10(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> exp(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_exp(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> exp2(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_exp2(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> sqrt(const simd<T, Dim1, Dim2> &x) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_sqrt(x.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> pow(const simd<T, Dim1, Dim2> &x,
                                  const simd<T, Dim1, Dim2> &y) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_pow(x.value_, y.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> max(const simd<T, Dim1, Dim2> &x,
                                  const simd<T, Dim1, Dim2> &y) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_max(x.value_, y.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> min(const simd<T, Dim1, Dim2> &x,
                                  const simd<T, Dim1, Dim2> &y) noexcept {
  return simd<T, Dim1, Dim2>{__builtin_elementwise_min(x.value_, y.value_)};
}

template <typename T, unsigned Dim1, unsigned Dim2>
constexpr simd<T, Dim1, Dim2> fma(const simd<T, Dim1, Dim2> &a,
                                  const simd<T, Dim1, Dim2> &b,
                                  const simd<T, Dim1, Dim2> &c) noexcept {
  return simd<T, Dim1, Dim2>{
      __builtin_elementwise_fma(a.value_, b.value_, c.value_)};
}
} // namespace aid