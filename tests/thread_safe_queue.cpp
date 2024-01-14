#include <catch2/catch_test_macros.hpp>

#include <list>

import aid.containers;

TEST_CASE("Basic methods work", "[thread_safe_queue]") {
  aid::thread_safe_queue<int> q;
  REQUIRE(q.empty());
  REQUIRE(!q.pop().has_value());
  q.push(1);
  q.push(2);
  q.push(3);
  q.push(4);
  q.push(5);
  REQUIRE(!q.empty());
  auto first = q.pop();
  REQUIRE(first.has_value());
  REQUIRE(*first == 1);
}
