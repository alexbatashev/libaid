#include <catch2/catch_test_macros.hpp>

import aid.memory;

TEST_CASE("Basic methods work", "[memory.memory_resource]") {
  aid::malloc_resource r;

  void *ptr = r.allocate(100, 32);
  REQUIRE(ptr != nullptr);
  std::optional<void *> realloc =
      r.reallocate(aid::trivial_mem_tag, ptr, 150, 32);

  if (realloc.has_value()) {
    REQUIRE(*realloc != nullptr);
    REQUIRE_NOTHROW(r.deallocate(*realloc, 150, 32));
  } else {
    REQUIRE_NOTHROW(r.deallocate(ptr, 100, 32));
  }
}
