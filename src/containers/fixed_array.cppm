module;

#include <cstddef>

export module aid.containers:fixed_array;

import aid.memory;
import :vector;

export namespace aid {
template <typename T>
class fixed_array : vector<T> {
public:
  using value_type = T;
  using reference = vector<value_type>::reference;
  using const_reference = vector<value_type>::const_reference;
  using iterator = vector<value_type>::iterator;
  using const_iterator = vector<value_type>::const_iterator;

  fixed_array(std::size_t size, const T &value = T()) {
    data_.resize(size, value);
  }

  reference operator[](std::size_t idx) noexcept { return data_[idx]; }

  const_reference operator[](std::size_t idx) const noexcept {
    return data_[idx];
  }

  iterator begin() noexcept { return data_.begin(); }

  iterator end() noexcept { return data_.end(); }

  const_iterator begin() const noexcept { return data_.begin(); }

  const_iterator end() const noexcept { return data_.end(); }

  std::size_t size() const noexcept { return data_.size(); }

private:
  vector<value_type> data_;
};
} // namespace aid