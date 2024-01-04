module;

#include <filesystem>
#include <system_error>

#ifndef _WIN32
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

export module aid.file;

import aid.execution;

namespace aid {
namespace fs = std::filesystem;

export using fs_size = uint64_t;

static_assert(sizeof(fs_size) >= sizeof(off_t),
              "System-defined file size is too big");

export class dma_file {
public:
  enum class mode { read, write, read_write };

  dma_file(const dma_file &) = delete;
  dma_file &operator=(const dma_file &) = delete;

  dma_file(dma_file &&other)
      : fd_(other.fd_), owning_dispatcher_(std::move(other.owning_dispatcher_)),
        dispatcher_(other.dispatcher_) {
    other.fd_ = -1;
  }
  dma_file &operator=(dma_file &&other) {
    fd_ = other.fd_;
    owning_dispatcher_ = std::move(other.owning_dispatcher_);
    dispatcher_ = other.dispatcher_;
    other.fd_ = -1;
    return *this;
  }

  dma_file(io_dispatcher *dispatcher, fs::path path, mode m = mode::read)
      : dispatcher_(dispatcher) {
    open_file(path, m);
  }

  dma_file(std::shared_ptr<io_dispatcher> dispatcher, fs::path path,
           mode m = mode::read)
      : owning_dispatcher_(std::move(dispatcher)) {
    dispatcher_ = owning_dispatcher_.get();
    open_file(path, m);
  }

  void close() noexcept {
    if (fd_ != -1) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  bool is_valid() const noexcept { return fd_ != -1; }

  fs_size size() const noexcept {
    struct stat stat;
    if (fstat(fd_, &stat) < 0)
      return static_cast<fs_size>(-1);

    if (S_ISBLK(stat.st_mode)) {
      unsigned long long bytes;
      if (ioctl(fd_, BLKGETSIZE64, &bytes) != 0) {
        return static_cast<fs_size>(-1);
      }
      return static_cast<fs_size>(bytes);
    } else if (S_ISREG(stat.st_mode))
      return static_cast<fs_size>(stat.st_size);

    return 0;
  }

  ~dma_file() { close(); }

private:
  void open_file(fs::path path, mode m) {
    int flags = O_DIRECT;
    switch (m) {
    case mode::read:
      flags |= O_RDONLY;
      break;
    case mode::write:
      flags |= O_WRONLY;
      break;
    case mode::read_write:
      flags |= O_RDWR;
      break;
    }

    fd_ = open(path.c_str(), flags);

    if (fd_ == -1) {
      int savedErrno = errno;
      throw fs::filesystem_error(
          "failed to open a file", path,
          std::error_code(savedErrno, std::generic_category()));
    }
  }

  int fd_;
  std::shared_ptr<io_dispatcher> owning_dispatcher_;
  io_dispatcher *dispatcher_;
};
} // namespace aid