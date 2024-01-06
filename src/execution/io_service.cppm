module;

#include <coroutine>
#include <filesystem>
#include <mutex>
#include <span>
#include <thread>

#ifdef __linux__
#include <liburing.h>
#include <sys/eventfd.h>
#endif

export module aid.execution:io_service;

import aid.mutex;
import aid.async;

namespace aid {
export struct io_size {
  uint64_t value;
};
export struct io_offset {
  uint64_t value;
};

export class io_result {
public:
  io_result(std::unique_ptr<manual_event> event) : event_(std::move(event)) {}

  auto operator co_await() {
    struct awaitable {
      awaitable(std::unique_ptr<manual_event> event)
          : event_(std::move(event)) {}

      bool await_ready() const noexcept { return event_->is_set(); }

      void await_suspend(std::coroutine_handle<> handle) const noexcept {
        // FIXME there must be a smarter thing to do
        event_->wait();
        handle.resume();
      }

      constexpr void await_resume() const noexcept {}

      std::unique_ptr<manual_event> event_;
    };

    return awaitable{std::move(event_)};
  }

private:
  std::unique_ptr<manual_event> event_;
};

export class io_service {
public:
  virtual void process_events() = 0;

  virtual void enable_poll_events() = 0;

  virtual void flush() = 0;

  virtual io_result read(int fd, std::span<std::byte> buffer,
                         io_offset offset) = 0;
  virtual io_result write(int fd, std::span<std::byte> buffer,
                          io_offset offset) = 0;

  virtual ~io_service() = default;
};

#ifdef __linux__

class uring {
public:
  // uring() = default; // invalid
  uring(int efd);

  void process_events() noexcept {
    std::unique_lock _{mutex_};
    io_uring_cqe *cqe;
    if (io_uring_wait_cqe(&ring_, &cqe) != 0) [[unlikely]]
      std::terminate();
    process_single_event(cqe);
    process_events_unsafe();
  }

  void flush_queue() noexcept {
    std::unique_lock _{mutex_};
    flush_queue_unsafe();
  }

  template <typename F>
  auto with_sqe(F &&f) {
    std::unique_lock _{mutex_};
    auto *sqe = get_sqe();
    return f(ring_, sqe);
  }

  ~uring() {
    std::unique_lock _{mutex_};
    io_uring_queue_exit(&ring_);
  }

private:
  [[nodiscard]] io_uring_sqe *get_sqe() noexcept;
  void process_events_unsafe() noexcept;
  void process_single_event(io_uring_cqe *cqe) noexcept;
  void flush_queue_unsafe() noexcept;

  spin_mutex mutex_;
  io_uring ring_;
};

export class uring_service final : public io_service {
public:
  uring_service() : ring_(efd_) {}

  void process_events() override { ring_.process_events(); }

  void enable_poll_events() override;

  void flush() override { ring_.flush_queue(); };

  io_result read(int fd, std::span<std::byte> buffer,
                 io_offset offset) override;
  io_result write(int fd, std::span<std::byte> buffer,
                  io_offset offset) override;

private:
  int efd_ = ::eventfd(0, 0);
  uring ring_;
  std::jthread poll_thread_;
};
#endif
} // namespace aid