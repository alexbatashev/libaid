// #include <catch2/catch_test_macros.hpp>

import aid.async:lock;

// #include "aid/async/lock.hpp"
// #include "aid/async/spin_mutex.hpp"

// #include <future>
// #include <mutex>

// template <aid::lockable M> void test_mutex() {
//   {
//     M m;
//     m.lock();
//     m.unlock();
//   }

//   {
//     int i = 0;
//     M m;
//     std::unique_lock lock{m};
//     auto f = std::async(std::launch::async, [&m, &i]() {
//       std::unique_lock _{m};
//       i = 42;
//     });

//     lock.unlock();
//     f.wait();

//     REQUIRE(i == 42);
//   }
// }

// template <aid::rw_lockable M> void test_rw_mutex() {
//   {
//     M mutex;
//     aid::shared_lock lock{mutex};
//     lock.upgrade();
//   }
//   {
//     int i = 0;
//     M m;
//     aid::shared_lock lock{m};
//     auto f = std::async(std::launch::async, [&m, &i]() {
//       aid::shared_lock threadLock{m};
//       threadLock.upgrade();
//       i = 42;
//     });

//     lock.unlock();
//     f.wait();

//     REQUIRE(i == 42);
//   }
// }

// TEST_CASE("Test spin_mutex", "[locks]") { test_mutex<aid::spin_mutex>(); }

// TEST_CASE("Test shared_spin_mutex", "[locks]") {
//   test_mutex<aid::shared_spin_mutex>();
//   test_rw_mutex<aid::shared_spin_mutex>();
// }
