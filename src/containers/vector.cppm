module;

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>

export module aid.containers:vector;

import aid.memory;

export namespace aid {
template <typename T>
class vector_base {
public:
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = const T &;
  using iterator = T *;
  using const_iterator = const T *;

  vector_base(const vector_base &) = delete;
  vector_base &operator=(const vector_base &) = delete;

  std::size_t size() const noexcept {
    if (mStart == nullptr || mEnd == nullptr)
      return 0;
    return std::distance(mStart, mEnd);
  }

  std::size_t capacity() const noexcept { return mCapacity; }

  reference operator[](std::size_t index) noexcept { return mStart[index]; }

  const_reference operator[](std::size_t index) const noexcept {
    return mStart[index];
  }

  iterator begin() noexcept { return mStart; }

  iterator end() noexcept { return mEnd; }

  const_iterator begin() const noexcept { return mStart; }

  const_iterator end() const noexcept { return mStart; }

  virtual ~vector_base() = default;

  reference front() noexcept { return mStart[0]; }

  reference back() noexcept { return mStart[size() - 1]; }

  const_reference front() const noexcept { return mStart[0]; }

  const_reference back() const noexcept { return mStart[size() - 1]; }

  virtual T &push_back(T &&element) = 0;

  virtual void reserve(std::size_t count) = 0;

  virtual void resize(std::size_t count, const T &value = T()) = 0;

protected:
  vector_base() = default;
  explicit vector_base(size_t capacity) : mCapacity(capacity){};

  iterator mStart = nullptr;
  iterator mEnd = nullptr;
  size_t mCapacity = 0;
};

template <typename T, std::size_t InlineSize = 5>
class inline_vector : public vector_base<T> {
  using super = vector_base<T>;

public:
  inline_vector() : super(InlineSize) {
    super::mStart = reinterpret_cast<T *>(mStackData.data());
    super::mEnd = reinterpret_cast<T *>(mStackData.data());
  }

  inline_vector(memory_resource *memResource)
      : super(InlineSize), mMemoryResource(memResource) {
    super::mStart = reinterpret_cast<T *>(mStackData.data());
    super::mEnd = reinterpret_cast<T *>(mStackData.data());
  }

  inline_vector(const inline_vector &other) {
    mMemoryResource = other.mMemoryResource;
    super::mCapacity = other.mCapacity;

    if (other.mStart == reinterpret_cast<const T *>(other.mStackData.data())) {
      std::copy(
          other.mStart, other.mEnd, reinterpret_cast<T *>(mStackData.data()));

      super::mStart = reinterpret_cast<T *>(mStackData.data());
      super::mEnd = super::mStart + other.size();
    } else {
      super::mStart = allocate(other.mCapacity);
      std::copy(other.mStart, other.mEnd, super::mStart);
      super::mEnd = super::mStart + other.size();
    }
  }

  inline_vector &operator=(const inline_vector &other) {
    mMemoryResource = other.mMemoryResource;
    super::mCapacity = other.mCapacity;

    if (other.mStart == reinterpret_cast<const T *>(other.mStackData.data())) {
      std::copy(
          other.mStart, other.mEnd, reinterpret_cast<T *>(mStackData.data()));

      super::mStart = reinterpret_cast<T *>(mStackData.data());
      super::mEnd = super::mStart + other.size();
    } else {
      super::mStart = allocate(other.mCapacity);
      std::copy(other.mStart, other.mEnd, super::mStart);
      super::mEnd = super::mStart + other.size();
    }

    return *this;
  }

  ~inline_vector() {
    if (super::mStart != reinterpret_cast<T *>(mStackData.data()))
      deallocate(super::mStart, super::mCapacity);
  }

  void reserve(std::size_t newCapacity) override {
    if (newCapacity < InlineSize)
      return;

    auto allocMove = [this, newCapacity] {
      T *newStart = allocate(newCapacity);

      std::move(super::mStart, super::mEnd, newStart);
      std::size_t numElements = super::size();
      super::mStart = newStart;
      super::mEnd = newStart + numElements;
    };

    if (super::mStart == reinterpret_cast<T *>(mStackData.data())) {
      allocMove();
    } else {
      std::optional<T *> maybePtr = reallocate(super::mStart, newCapacity);

      if (!maybePtr) {
        allocMove();
      } else {
        std::size_t numElements = super::size();
        super::mStart = *maybePtr;
        super::mEnd = *maybePtr + numElements;
      }
    }

    super::mCapacity = newCapacity;
  }

  void resize(std::size_t newCapacity, const T &value) override {
    auto init = [this, &value](T *start, T *end) {
      for (auto it = start; it != end; ++it)
        new (it) T(value);
      super::mEnd = end;
    };

    auto deinit = [this](T *start, T *end) {
      for (auto it = start; it != end; ++it)
        std::destroy_at(it);
      super::mEnd = start;
    };

    if (super::size() > newCapacity) {
      std::size_t diff = super::size() - newCapacity;
      deinit(super::mEnd - diff, super::mEnd);
      return;
    }

    std::size_t diff = newCapacity - super::size();
    if (newCapacity < InlineSize) {
      init(super::mEnd, super::mEnd + diff);
      return;
    }

    auto allocMove = [this, newCapacity] {
      T *newStart = allocate(newCapacity);

      std::move(super::mStart, super::mEnd, newStart);
      std::size_t numElements = super::size();
      super::mStart = newStart;
      super::mEnd = newStart + numElements;
    };

    if (super::mStart == reinterpret_cast<T *>(mStackData.data())) {
      allocMove();
    } else {
      std::optional<T *> maybePtr = reallocate(super::mStart, newCapacity);

      if (!maybePtr) {
        allocMove();
      } else {
        std::size_t numElements = super::size();
        super::mStart = *maybePtr;
        super::mEnd = *maybePtr + numElements;
      }
    }

    init(super::mEnd, super::mEnd + diff);

    super::mCapacity = newCapacity;
  }

  T &push_back(T &&element) override {
    std::size_t numElements = super::size();
    if (numElements == super::mCapacity)
      reserve(super::mCapacity + super::mCapacity / 2);

    super::mStart[numElements] = std::forward<T>(element);
    super::mEnd += 1;

    return super::mStart[numElements];
  }

  template <typename... Ts>
  T &emplace_back(Ts &&...args) {
    std::size_t numElements = super::size();
    if (numElements == super::mCapacity)
      reserve(super::mCapacity + super::mCapacity / 2);

    new (super::mStart + numElements) T(std::forward<Ts>(args)...);
    super::mEnd += 1;

    return super::mStart[numElements];
  }

private:
  T *allocate(std::size_t count) {
    auto withResource = [count](memory_resource *resource) {
      return static_cast<T *>(
          resource->allocate(count * sizeof(T), alignof(T)));
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }
    malloc_resource r;
    return withResource(&r);
  }

  void deallocate(void *ptr, std::size_t count) {
    auto withResource = [count, ptr](memory_resource *resource) {
      resource->deallocate(ptr, count * sizeof(T), alignof(T));
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }
    malloc_resource r;
    return withResource(&r);
  }

  std::optional<T *> reallocate(void *oldPtr, std::size_t count) {
    auto withResource =
        [count, oldPtr](memory_resource *resource) -> std::optional<T *> {
      if constexpr (std::is_trivially_copyable_v<T>) {
        auto maybePtr = resource->reallocate(
            trivial_mem_tag, oldPtr, count * sizeof(T), alignof(T));
        if (maybePtr)
          return static_cast<T *>(*maybePtr);
      } else {
        auto maybePtr =
            resource->reallocate(oldPtr, count * sizeof(T), alignof(T));
        if (maybePtr)
          return static_cast<T *>(*maybePtr);
      }
      return std::nullopt;
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }

    malloc_resource r;
    return withResource(&r);
  }

  std::array<std::byte, InlineSize * sizeof(T)> mStackData;
  memory_resource *mMemoryResource = nullptr;
};

template <typename T>
class vector : public vector_base<T> {
  using super = vector_base<T>;

public:
  vector() = default;

  vector(memory_resource *memResource) : mMemoryResource(memResource) {}

  vector(const vector &other) {
    mMemoryResource = other.mMemoryResource;
    super::mCapacity = other.mCapacity;

    if (other.mStart != nullptr) {
      super::mStart = allocate(other.mCapacity);
      std::copy(other.mStart, other.mEnd, super::mStart);
      super::mEnd = super::mStart + other.size();
    }
  }

  vector &operator=(const vector &other) {
    mMemoryResource = other.mMemoryResource;
    super::mCapacity = other.mCapacity;

    if (other.mStart == nullptr) {
      super::mStart = allocate(other.mCapacity);
      std::copy(other.mStart, other.mEnd, super::mStart);
      super::mEnd = super::mStart + other.size();
    }

    return *this;
  }

  ~vector() {
    if (super::mStart != nullptr)
      deallocate(super::mStart, super::mCapacity);
  }

  void reserve(std::size_t newCapacity) override {
    if (newCapacity == 0)
      return;

    auto allocMove = [this, newCapacity] {
      T *newStart = allocate(newCapacity);

      std::move(super::mStart, super::mEnd, newStart);
      std::size_t numElements = super::size();
      super::mStart = newStart;
      super::mEnd = newStart + numElements;
    };

    if (super::mStart == nullptr) {
      allocMove();
    } else {
      std::optional<T *> maybePtr = reallocate(super::mStart, newCapacity);

      if (!maybePtr) {
        allocMove();
      } else {
        std::size_t numElements = super::size();
        super::mStart = *maybePtr;
        super::mEnd = *maybePtr + numElements;
      }
    }

    super::mCapacity = newCapacity;
  }

  void resize(std::size_t newCapacity, const T &value) override {
    auto init = [this, &value](T *start, T *end) {
      for (auto it = start; it != end; ++it)
        new (it) T(value);
      super::mEnd = end;
    };

    auto deinit = [this](T *start, T *end) {
      for (auto it = start; it != end; ++it)
        std::destroy_at(it);
      super::mEnd = start;
    };

    if (super::size() > newCapacity) {
      std::size_t diff = super::size() - newCapacity;
      deinit(super::mEnd - diff, super::mEnd);
      return;
    }

    std::size_t diff = newCapacity - super::size();

    auto allocMove = [this, newCapacity] {
      T *newStart = allocate(newCapacity);

      std::move(super::mStart, super::mEnd, newStart);
      std::size_t numElements = super::size();
      super::mStart = newStart;
      super::mEnd = newStart + numElements;
    };

    if (super::mStart == nullptr) {
      allocMove();
    } else {
      std::optional<T *> maybePtr = reallocate(super::mStart, newCapacity);

      if (!maybePtr) {
        allocMove();
      } else {
        std::size_t numElements = super::size();
        super::mStart = *maybePtr;
        super::mEnd = *maybePtr + numElements;
      }
    }

    init(super::mEnd, super::mEnd + diff);

    super::mCapacity = newCapacity;
  }

  T &push_back(T &&element) override {
    if (super::mCapacity == 0)
      reserve(16);
    std::size_t numElements = super::size();
    if (numElements == super::mCapacity)
      reserve(super::mCapacity + super::mCapacity / 2);

    super::mStart[numElements] = std::forward<T>(element);
    super::mEnd += 1;

    return super::mStart[numElements];
  }

  template <typename... Ts>
  T &emplace_back(Ts &&...args) {
    if (super::mCapacity == 0)
      reserve(16);
    std::size_t numElements = super::size();
    if (numElements == super::mCapacity)
      reserve(super::mCapacity + super::mCapacity / 2);

    new (super::mStart + numElements) T(std::forward<Ts>(args)...);
    super::mEnd += 1;

    return super::mStart[numElements];
  }

private:
  T *allocate(std::size_t count) {
    auto withResource = [count](memory_resource *resource) {
      return static_cast<T *>(
          resource->allocate(count * sizeof(T), alignof(T)));
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }
    malloc_resource r;
    return withResource(&r);
  }

  void deallocate(void *ptr, std::size_t count) {
    auto withResource = [count, ptr](memory_resource *resource) {
      resource->deallocate(ptr, count * sizeof(T), alignof(T));
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }
    malloc_resource r;
    return withResource(&r);
  }

  std::optional<T *> reallocate(void *oldPtr, std::size_t count) {
    auto withResource =
        [count, oldPtr](memory_resource *resource) -> std::optional<T *> {
      if constexpr (std::is_trivially_copyable_v<T>) {
        auto maybePtr = resource->reallocate(
            trivial_mem_tag, oldPtr, count * sizeof(T), alignof(T));
        if (maybePtr)
          return static_cast<T *>(*maybePtr);
      } else {
        auto maybePtr =
            resource->reallocate(oldPtr, count * sizeof(T), alignof(T));
        if (maybePtr)
          return static_cast<T *>(*maybePtr);
      }
      return std::nullopt;
    };

    if (mMemoryResource) {
      return withResource(mMemoryResource);
    }

    malloc_resource r;
    return withResource(&r);
  }

  [[no_unique_address]] memory_resource *mMemoryResource = nullptr;
};
} // namespace aid