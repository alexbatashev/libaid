#include <catch2/catch_test_macros.hpp>

import aid.execution;
import aid.file;

TEST_CASE("io_uring dispatcher", "[execution.uring_dispatcher]") {
  aid::uring_dispatcher d;
}