#include <catch2/catch_test_macros.hpp>

import aid.containers;

TEST_CASE("Basic lock free deque operation", "[containers][lock_free_deque]") {
  aid::lock_free_deque<int> deque{256};

  deque.push_back(1);
  deque.push_back(2);
  deque.push_back(3);

  auto last = deque.pop_back();
  REQUIRE(last.has_value());
  REQUIRE(*last == 3);

  auto first = deque.pop_front();
  REQUIRE(first.has_value());
  REQUIRE(*first == 1);

  auto remaining = deque.pop_back();
  REQUIRE(remaining.has_value());
  REQUIRE(*remaining == 2);

  REQUIRE(!deque.pop_back().has_value());
  REQUIRE(!deque.pop_front().has_value());
}