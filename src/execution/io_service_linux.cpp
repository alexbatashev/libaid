module;

#include <liburing.h>
#include <mutex>
#include <span>
#include <stop_token>
#include <sys/epoll.h>
#include <thread>

module aid.execution;

import aid.async;

using namespace aid;

aid::uring::uring(int efd) {
  struct io_uring_params params;

  memset(&params, 0, sizeof(params));
  // FIXME(alexbatashev): add a flag to allow polling
  // params.flags |= IORING_SETUP_SQPOLL;
  // params.sq_thread_idle = 2000;

  if (io_uring_queue_init_params(256, &ring_, &params) != 0)
    throw std::runtime_error("failed to initialize io_uring");

  if (io_uring_register_eventfd(&ring_, efd))
    throw std::runtime_error("failed to register eventfd with io_uring");
}

io_uring_sqe *aid::uring::get_sqe() noexcept {
  auto *sqe = io_uring_get_sqe(&ring_);
  if (sqe)
    return sqe;
  else {
    process_events_unsafe();
    flush_queue_unsafe();
    sqe = io_uring_get_sqe(&ring_);
    if (sqe == nullptr) [[unlikely]]
      std::terminate(); // something bad happened, can't recover
  }

  return sqe;
}

void aid::uring::flush_queue_unsafe() noexcept { io_uring_submit(&ring_); }

void aid::uring::process_events_unsafe() noexcept {
  std::vector<io_uring_cqe *> cqes{256, nullptr};

  unsigned count = io_uring_peek_batch_cqe(&ring_, cqes.data(), 256);

  for (unsigned i = 0; i < count; i++) {
    process_single_event(cqes[i]);
  }
}

void aid::uring::process_single_event(io_uring_cqe *cqe) noexcept {
  auto *event = static_cast<manual_event *>(io_uring_cqe_get_data(cqe));
  event->set();
  io_uring_cqe_seen(&ring_, cqe);
}

io_result aid::uring_service::read(int fd, std::span<std::byte> buffer,
                                   io_offset offset) {
  auto event = std::make_unique<manual_event>();
  ring_.with_sqe(
      [fd, buffer, offset, &event](io_uring &ring, io_uring_sqe *sqe) {
        io_uring_prep_read(sqe, fd, static_cast<void *>(buffer.data()),
                           buffer.size(), offset.value);
        io_uring_sqe_set_data(sqe, event.get());
      });

  return io_result{std::move(event)};
}

io_result aid::uring_service::write(int fd, std::span<std::byte> buffer,
                                    io_offset offset) {
  auto event = std::make_unique<manual_event>();
  ring_.with_sqe(
      [fd, buffer, offset, &event](io_uring &ring, io_uring_sqe *sqe) {
        io_uring_prep_write(sqe, fd, static_cast<void *>(buffer.data()),
                            buffer.size(), offset.value);
        io_uring_sqe_set_data(sqe, event.get());
      });

  return io_result{std::move(event)};
}

void aid::uring_service::enable_poll_events() {
  // FIXME: this has a chance of blocking forever
  poll_thread_ = std::jthread([this](std::stop_token st) {
    std::vector<epoll_event> evts;
    evts.resize(256);
    while (!st.stop_requested()) {
      epoll_wait(efd_, evts.data(), 256, 100);
      if (st.stop_requested())
        break;
      ring_.process_events();
    }
  });
}