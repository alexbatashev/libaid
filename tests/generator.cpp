#include <catch2/catch_test_macros.hpp>

#include "aid/async/generator.hpp"

aid::generator<int> generate_ints() {
  co_yield 1;
  co_yield 2;
  co_yield 3;
}

struct pair {
  size_t a;
  size_t b;
};

aid::generator<pair> generate_pairs() {
  co_yield {1, 2};
  co_yield {3, 4};
  co_yield {5, 6};
}

TEST_CASE("Can fetch integers from generator", "[generator]") {
  std::vector<int> ints;

  for (int i : generate_ints()) {
    ints.push_back(i);
  }

  REQUIRE(ints.size() == 3);
  REQUIRE(ints[0] == 1);
  REQUIRE(ints[1] == 2);
  REQUIRE(ints[2] == 3);
}

TEST_CASE("Can fetch pairs from generator", "[generator]") {
  std::vector<pair> pairs;

  for (auto p : generate_pairs()) {
    pairs.push_back(p);
  }

  REQUIRE(pairs.size() == 3);
  REQUIRE(pairs[0].a == 1);
  REQUIRE(pairs[0].b == 2);
  REQUIRE(pairs[1].a == 3);
  REQUIRE(pairs[1].b == 4);
  REQUIRE(pairs[2].a == 5);
  REQUIRE(pairs[2].b == 6);
}
