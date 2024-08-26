#pragma once

#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>
namespace co_io {

class ByteBuffer {
public:
  ByteBuffer() = default;
  explicit ByteBuffer(size_t size) : buf_(size) {}

  ByteBuffer(const ByteBuffer &) = delete;
  ByteBuffer &operator=(const ByteBuffer &) = delete;

  ByteBuffer(ByteBuffer &&) = default;
  ByteBuffer &operator=(ByteBuffer &&) = default;

  const char *data() const noexcept { return buf_.data(); }
  char *data() noexcept { return buf_.data(); }

  size_t size() const noexcept { return buf_.size(); }

  char *begin() noexcept { return buf_.data(); }
  const char *begin() const noexcept { return buf_.data(); }

  char *end() noexcept { return buf_.data() + buf_.size(); }
  const char *end() const noexcept { return buf_.data() + buf_.size(); }

  std::string_view span(size_t start, size_t len) const {
    if (start > size()) {
      throw std::out_of_range("ByteBuffer::span");
    }
    len = std::min(len, size() - start);
    return std::string_view{begin() + start, len};
  }

  operator std::string_view() const { return span(0, size()); }

  void append(std::string_view str) {
    buf_.insert(buf_.end(), str.begin(), str.end());
  }

  void clear() { buf_.clear(); }

  template <size_t N> void append(const char (&str)[N]) {
    append(std::string_view{str, N - 1});
  }

private:
  std::vector<char> buf_;
};

} // namespace co_io
