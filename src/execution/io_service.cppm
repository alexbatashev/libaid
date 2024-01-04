module;

#include <coroutine>
#include <filesystem>
#include <span>

#ifdef __linux__
#include <liburing.h>
#endif

export module aid.execution:io_service;

namespace aid {
export struct io_size {
  uint64_t value;
};
export struct io_offset {
  uint64_t value;
};

export class io_result {
public:
  io_result(std::function<void()> wait) : wait_(std::move(wait)) {}

  auto operator co_await() {
    struct awaitable {
      awaitable(std::function<void()> wait) : wait_(std::move(wait)) {}

      constexpr bool await_ready() const noexcept { return false; }

      void await_suspend(std::coroutine_handle<> handle) const noexcept {
        handle_ = handle;
        wait_();
      }

      constexpr void await_resume() const noexcept {}
    };

    return awaitable{std::move(wait)};
  }

private:
  void *user_data_;
  void (*suspend_callback_)(std::coroutine_handle<>, void *);
  std::coroutine_handle<> handle_;
};

export class io_dispatcher {
public:
  virtual void register_files(std::span<int> fds) = 0;

  virtual io_result read(int fd, std::span<std::byte> buffer,
                         io_offset offset) = 0;

  virtual ~io_dispatcher() = default;
};

#ifdef __linux__
export class uring_dispatcher final : public io_dispatcher {
public:
  uring_dispatcher();

  void register_files(std::span<int> fd) override;

  virtual io_result read(int fd, std::span<std::byte> buffer, io_offset offset);

  ~uring_dispatcher();

private:
  [[nodiscard]] struct io_uring_sqe *get_sqe() noexcept;

  struct io_uring ring_;
  size_t cqe_count_;
};
#endif
} // namespace aid