module;

#include <coroutine>
#include <exception>
#include <type_traits>

export module aid.async:generator;

namespace aid {
namespace detail {
template <typename T> struct yield_traits {
  using type = std::conditional_t<(sizeof(T) > sizeof(void *)), T *, T>;

  static constexpr bool is_byref() noexcept {
    return sizeof(T) > sizeof(void *);
  }
};
} // namespace detail
export template <typename T> class [[nodiscard]] generator {
public:
  struct promise_type;
  using handle_type = std::coroutine_handle<promise_type>;

  constexpr explicit generator(handle_type h) : mHandle(h) {}
  generator(const generator &) = delete;
  generator &operator=(const generator &) = delete;
  generator(generator &&other) noexcept : mHandle(std::move(other.mHandle)) {
    other.mHandle = nullptr;
  }
  generator &operator=(generator &&other) noexcept {
    if (this != &other) {
      if (mHandle) {
        mHandle.destroy();
      }
      mHandle = std::move(other.mHandle);
      other.mHandle = nullptr;
    }
    return *this;
  }
  ~generator() {
    if (mHandle) {
      mHandle.destroy();
      mHandle = nullptr;
    }
  }

  struct promise_type {
    constexpr promise_type() noexcept = default;
    ~promise_type() = default;
    promise_type(const promise_type &) = delete;
    promise_type(promise_type &&) = delete;
    promise_type &operator=(const promise_type &) = delete;
    promise_type &operator=(promise_type &&) = delete;

    auto get_return_object() {
      return generator{handle_type::from_promise(*this)};
    }

    static constexpr std::suspend_always initial_suspend() noexcept {
      return {};
    }

    static constexpr std::suspend_always final_suspend() noexcept { return {}; }

    void return_void() {}

    auto yield_value(T &&x) {
      if constexpr (detail::yield_traits<T>::is_byref()) {
        mCurrentValue = std::addressof(x);
      } else {
        mCurrentValue = x;
      }
      return std::suspend_always{};
    }

    void unhandled_exception() { std::terminate(); }

  private:
    friend generator;
    typename detail::yield_traits<T>::type mCurrentValue;
  };

  struct iterator {
    using difference_type = std::ptrdiff_t;
    using value_type = T;

    constexpr iterator() noexcept = default;
    constexpr iterator(handle_type handle) : mHandle(handle) {}

    const T &operator*() const {
      if constexpr (detail::yield_traits<T>::is_byref()) {
        return *mHandle.promise().mCurrentValue;
      } else {
        return mHandle.promise().mCurrentValue;
      }
    }

    iterator &operator++() {
      next();
      return *this;
    }

    void operator++(int) { ++*this; }

    bool operator==(const iterator &it) const = default;

  private:
    friend generator;

    void next() {
      if (mHandle) {
        mHandle.resume();
        if (mHandle.done())
          mHandle = nullptr;
      }
    }

    handle_type mHandle;
  };

  iterator begin() const {
    if (!mHandle || mHandle.done()) {
      return iterator{nullptr};
    }

    iterator it{mHandle};
    it.next();
    return it;
  }

  iterator end() const { return iterator{nullptr}; }

private:
  handle_type mHandle;
};
} // namespace aid
