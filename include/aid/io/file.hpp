#pragma once

#include <filesystem>

namespace aid {
struct read_result {};
using file_offset_type = uint64_t;

namespace detail {
class file_impl {
public:
  virtual read_result read_at(file_offset_type offset, size_t size) = 0;
};
} // namespace detail

class file {
public:
  file(std::filesystem::path path) {}

  auto read_at(file_offset_type offset, size_t size) -> read_result {
    return mFile->read_at(offset, size);
  }

private:
  std::unique_ptr<detail::file_impl> mFile;
};
} // namespace aid