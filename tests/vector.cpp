#include <catch2/catch_test_macros.hpp>

import aid.containers;

TEST_CASE("Basic inline vector", "[containers][inline_vector]") {
  aid::inline_vector<int, 5> vec;

  REQUIRE(vec.size() == 0);
  REQUIRE(vec.capacity() == 5);
  REQUIRE(vec.push_back(42) == 42);
  REQUIRE(vec.size() == 1);

  aid::inline_vector<int, 5> stackCopy = vec;
  REQUIRE(stackCopy.capacity() == 5);
  REQUIRE(stackCopy.size() == 1);
  REQUIRE(stackCopy[0] == 42);

  REQUIRE(vec.emplace_back(1) == 1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(5);

  REQUIRE(vec.capacity() == 7);
  REQUIRE(vec.size() == 6);

  vec.reserve(100);
  REQUIRE(vec.size() == 6);
  REQUIRE(vec.capacity() == 100);
  REQUIRE(vec[0] == 42);
  REQUIRE(vec.front() == 42);
  REQUIRE(vec[5] == 5);
  REQUIRE(vec.back() == 5);
  vec[0] = 0;
  REQUIRE(vec[0] == 0);

  aid::inline_vector<int, 5> heapCopy = vec;
  REQUIRE(heapCopy.capacity() == 100);
  REQUIRE(heapCopy.size() == 6);
  REQUIRE(heapCopy[0] == 0);
  REQUIRE(heapCopy[1] == 1);

  aid::vector_base<int> &base = vec;
  REQUIRE(base.front() == 0);
  base.front() = 42;
  REQUIRE(vec.front() == 42);
}

TEST_CASE("Basic vector", "[containers][vector]") {
  aid::vector<int> vec;

  REQUIRE(vec.size() == 0);
  REQUIRE(vec.capacity() == 0);
  REQUIRE(vec.push_back(42) == 42);
  REQUIRE(vec.size() == 1);
  REQUIRE(vec.capacity() == 16);

  REQUIRE(vec.emplace_back(1) == 1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(5);

  for (int i = 0; i < 11; i++) {
    vec.push_back(42);
  }

  REQUIRE(vec.capacity() == 24);
  REQUIRE(vec.size() == 17);

  vec.reserve(100);
  REQUIRE(vec.size() == 17);
  REQUIRE(vec.capacity() == 100);
  REQUIRE(vec[0] == 42);
  REQUIRE(vec.front() == 42);
  REQUIRE(vec[5] == 5);
  REQUIRE(vec.back() == 42);
  vec[0] = 0;
  REQUIRE(vec[0] == 0);

  aid::vector<int> heapCopy = vec;
  REQUIRE(heapCopy.capacity() == 100);
  REQUIRE(heapCopy.size() == 17);
  REQUIRE(heapCopy[0] == 0);
  REQUIRE(heapCopy[1] == 1);

  aid::vector_base<int> &base = vec;
  REQUIRE(base.front() == 0);
  base.front() = 42;
  REQUIRE(vec.front() == 42);
}
