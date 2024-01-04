#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <coroutine>
#include <future>
#include <mutex>
#include <thread>

import aid.async;
import aid.execution;

TEST_CASE("Create default thread pool", "[thread_pool]") {
  aid::thread_pool tp;
  REQUIRE(tp.get_num_threads() == std::thread::hardware_concurrency());
}

TEST_CASE("Create static thread pool", "[thread_pool]") {
  aid::thread_pool tp{42};
  REQUIRE(tp.get_num_threads() == 42);
}

aid::task<std::thread::id> sample_task(aid::thread_pool &tp) {
  using namespace std::chrono_literals;

  co_await tp.schedule();

  std::this_thread::sleep_for(10ms);

  co_return std::this_thread::get_id();
}

TEST_CASE("Successfully schedule work", "[thread_pool]") {
  aid::thread_pool tp{2};

  auto task = sample_task(tp);

  std::thread::id value = aid::sync_wait(std::move(task));

  REQUIRE(value != std::this_thread::get_id());
}