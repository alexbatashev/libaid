#include <benchmark/benchmark.h>

#include <coroutine>
#include <stop_token>
#include <thread>

import aid.containers;

aid::thread_safe_queue<int> *g_test_queue;

static void thread_safe_queue_push(benchmark::State &state) {
  if (state.thread_index() == 0)
    g_test_queue = new aid::thread_safe_queue<int>();

  for (auto _ : state) {
    g_test_queue->push(42);
  }

  if (state.thread_index() == 0)
    delete g_test_queue;
}

static void thread_safe_queue_mpmc(benchmark::State &state) {
  if (state.thread_index() == 0)
    g_test_queue = new aid::thread_safe_queue<int>();

  for (auto _ : state) {
    if (state.thread_index() % 2 == 0)
      g_test_queue->push(42);
    else {
      auto tmp = g_test_queue->pop();
      benchmark::DoNotOptimize(tmp);
    }
  }

  if (state.thread_index() == 0)
    delete g_test_queue;
}

std::jthread *g_consumer_thread;

static void thread_safe_queue_mpsc(benchmark::State &state) {
  if (state.thread_index() == 0) {
    g_test_queue = new aid::thread_safe_queue<int>();
    g_consumer_thread = new std::jthread([](std::stop_token t) {
      while (!t.stop_requested()) {
        g_test_queue->wait(t);

        g_test_queue->pop();
      }
    });
  }

  for (auto _ : state) {
    g_test_queue->push(42);
  }

  if (state.thread_index() == 0) {
    g_consumer_thread->request_stop();
    delete g_consumer_thread;
    delete g_test_queue;
  }
}

BENCHMARK(thread_safe_queue_push)->ThreadRange(1, 12);
BENCHMARK(thread_safe_queue_mpmc)->ThreadRange(2, 12);
BENCHMARK(thread_safe_queue_mpsc)->ThreadRange(1, 12);