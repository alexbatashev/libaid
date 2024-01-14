#include <benchmark/benchmark.h>

#include <coroutine>

import aid.async;
import aid.execution;

aid::async_task<size_t> chain_coro(aid::thread_pool &tp, size_t max_depth, size_t cur_depth = 0) {
    co_await tp.schedule();

    if (cur_depth == max_depth) {
        co_return max_depth;
    }

    size_t i = co_await chain_coro(tp, max_depth, cur_depth + 1);

    co_return i;
}

aid::async_task<int> single_task(aid::thread_pool &tp) {
    co_await tp.schedule();

    co_return 42;
}

static void threading_chain_spawn(benchmark::State &state) {
    aid::thread_pool tp;

    for (auto _ : state) {
        auto coro = chain_coro(tp, state.range(0));
        size_t i = aid::sync_wait(std::move(coro));
        benchmark::DoNotOptimize(i);
    }
}

static void threading_sequential_spawn(benchmark::State &state) {
    aid::thread_pool tp;

    std::vector<aid::async_task<int>> tasks;
    tasks.reserve(state.range(0));

    for (size_t i = 0; i < state.range(0); i++) {
        auto task = single_task(tp);
        tasks.push_back(std::move(task));
    }

    for (auto _ : state) {
        for (auto &t : tasks) {
            aid::sync_wait(std::move(t));
        }
    }
}

BENCHMARK(threading_chain_spawn)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMillisecond);
BENCHMARK(threading_sequential_spawn)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMillisecond);