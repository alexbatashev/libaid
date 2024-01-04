module;

#include <liburing.h>
#include <span>

module aid.execution;

using namespace aid;

struct io_uring_sqe *aid::uring_service::get_sqe() noexcept {
  auto *sqe = io_uring_get_sqe(&ring_);
  if (sqe)
    return sqe;
  else {
    io_uring_cq_advance(&ring_, cqe_count_);
    cqe_count_ = 0;
    io_uring_submit(&ring_);
    sqe = io_uring_get_sqe(&ring_);
    if (sqe == nullptr) [[unlikely]]
      std::terminate(); // something bad happened, can't recover
  }

  return sqe;
}

aid::uring_dispatcher::uring_dispatcher() {
  struct io_uring_params params;

  memset(&params, 0, sizeof(params));
  // FIXME(alexbatashev): add a flag to allow polling
  // params.flags |= IORING_SETUP_SQPOLL;
  // params.sq_thread_idle = 2000;

  int ret = io_uring_queue_init_params(8, &ring_, &params);
  if (ret != 0)
    throw std::runtime_error("failed to initialize io_uring");
}

void aid::uring_dispatcher::register_files(std::span<int> fds) {
  if (io_uring_register_files(ring_, fds.data(), fds.size()) != 0) {
    throw std::runtime_error("failed to register file descriptors");
  }
}

io_result aid::uring_dispatcher::read(int fd, std::span<std::byte> buffer,
                                      io_offset offset) {
  auto *sqe = get_sqe();
  io_uring_prep_read(sqe, fd, static_cast<void *>(buffer.data()), buffer.size(),
                     offset);
  return {};
}

aid::uring_dispatcher::~uring_dispatcher() { io_uring_queue_exit(&ring_); }