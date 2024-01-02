module;

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <optional>

#ifdef _WIN32
#include "malloc.h"
#endif

export module aid.memory:memory_resource;

export namespace aid {

struct trivial_mem_tag_t {};
constexpr inline trivial_mem_tag_t trivial_mem_tag{};

class memory_resource : public std::pmr::memory_resource {
public:
  std::optional<void *>
  reallocate(void *ptr, std::size_t bytes,
             std::size_t alignment = alignof(std::max_align_t)) {
    return do_reallocate(ptr, bytes, alignment, /*trivial_copy_allowed=*/false);
  }

  std::optional<void *>
  reallocate(trivial_mem_tag_t, void *ptr, std::size_t bytes,
             std::size_t alignment = alignof(std::max_align_t)) {
    return do_reallocate(ptr, bytes, alignment, /*trivial_copy_allowed=*/true);
  }

protected:
  virtual std::optional<void *> do_reallocate(void *ptr, std::size_t bytes,
                                              std::size_t alignment,
                                              bool trivial_copy_allowed) = 0;
};

class malloc_resource final : public memory_resource {
protected:
  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
    return std::aligned_alloc(alignment, bytes);
  }

  void do_deallocate(void *ptr, [[maybe_unused]] std::size_t bytes,
                     [[maybe_unused]] std::size_t alignment) override {
    std::free(ptr);
  }

  std::optional<void *>
  do_reallocate([[maybe_unused]] void *ptr, [[maybe_unused]] std::size_t bytes,
                [[maybe_unused]] std::size_t alignment,
                [[maybe_unused]] bool trivial_copy_allowed) override {
#ifdef _WIN32
    if (trivial_copy_allowed) {
      return _aligned_realloc(ptr, bytes, alignment);
    }
#endif

    return std::nullopt;
  }

  bool
  do_is_equal(const std::pmr::memory_resource &other) const noexcept override {
    if (this == &other)
      return true;

    if (const auto *maybe = dynamic_cast<const malloc_resource *>(&other);
        maybe != nullptr)
      return true;

    return false;
  }
};
} // namespace aid