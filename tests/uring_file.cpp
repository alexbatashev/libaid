#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <vector>

import aid.execution;
import aid.file;

TEST_CASE("io_uring dispatcher", "[execution.uring_dispatcher]") {
  aid::uring_service io;

  aid::dma_file rnd{&io, "/dev/random", aid::dma_file::mode::read};

  std::vector<std::byte> ints{256, std::byte{0}};
  auto evt = rnd.read_at(0, ints);
  io.flush();
  io.process_events();

  // FIXME: this part is flaky, need to come up with a more robust test
  REQUIRE(static_cast<int>(ints[0]) != 0);
}