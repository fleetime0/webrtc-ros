#include "common/v4l2_frame_buffer.h"
#include "common/logging.h"

#include <cstring>
#include <libyuv.h>

// Aligning pointer to 64 bytes for improved performance, e.g. use SIMD.
static const int kBufferAlignment = 64;

static inline std::size_t AlignUp(std::size_t value, std::size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

std::shared_ptr<I420Buffer> I420Buffer::Create(int width, int height, int align) {
  if (width <= 0 || height <= 0)
    throw std::invalid_argument("I420Buffer: invalid size");
  if (align <= 0 || (align & (align - 1)))
    throw std::invalid_argument("I420Buffer: alignment must be power of two");

  const int stride_y = std::max(AlignUp(width, align), 16);
  const int stride_u = stride_y / 2;
  const int stride_v = stride_y / 2;

  if (size_t(stride_y) > SIZE_MAX / size_t(height))
    throw std::overflow_error("I420Buffer: size overflow (Y)");

  const int chroma_h = (height + 1) / 2;

  if (size_t(stride_u) > SIZE_MAX / size_t(chroma_h))
    throw std::overflow_error("I420Buffer: size overflow (U/V)");

  const size_t y_bytes = size_t(stride_y) * size_t(height);
  const size_t u_bytes = size_t(stride_u) * size_t(chroma_h);
  const size_t v_bytes = u_bytes;

  size_t total_raw = y_bytes;
  if (total_raw > SIZE_MAX - u_bytes)
    throw std::overflow_error("I420Buffer: size overflow (Y+U)");
  total_raw += u_bytes;
  if (total_raw > SIZE_MAX - v_bytes)
    throw std::overflow_error("I420Buffer: size overflow (Y+U+V)");
  total_raw += v_bytes;

  const size_t total = (total_raw + align - 1) / align * align;

  auto mem = std::unique_ptr<uint8_t, BoostAlignedFree>(
          static_cast<uint8_t *>(boost::alignment::aligned_alloc(align, total)), BoostAlignedFree{});
  if (!mem)
    throw std::bad_alloc();

  auto buf = std::shared_ptr<I420Buffer>(new I420Buffer(width, height, stride_y, stride_u, stride_v, align));
  buf->mem_ = std::move(mem);
  buf->y_ = buf->mem_.get();
  buf->u_ = buf->y_ + y_bytes;
  buf->v_ = buf->u_ + u_bytes;
  return buf;
}

std::shared_ptr<V4L2FrameBuffer> V4L2FrameBuffer::Create(int width, int height, int size, uint32_t format) {
  return std::make_shared<V4L2FrameBuffer>(width, height, size, format);
}

std::shared_ptr<V4L2FrameBuffer> V4L2FrameBuffer::Create(int width, int height, V4L2Buffer buffer) {
  return std::make_shared<V4L2FrameBuffer>(width, height, buffer);
}

V4L2FrameBuffer::V4L2FrameBuffer(int width, int height, V4L2Buffer buffer) :
    width_(width), height_(height), format_(buffer.pix_fmt), size_(buffer.length), flags_(buffer.flags),
    is_buffer_copied(false), timestamp_(buffer.timestamp), buffer_(buffer),
    data_(static_cast<uint8_t *>(boost::alignment::aligned_alloc(
                  kBufferAlignment, AlignUp(static_cast<std::size_t>(size_), kBufferAlignment))),
          BoostAlignedFree{}) {}

V4L2FrameBuffer::V4L2FrameBuffer(int width, int height, int size, uint32_t format) :
    width_(width), height_(height), format_(format), size_(size), flags_(0), is_buffer_copied(false),
    timestamp_({0, 0}), data_(static_cast<uint8_t *>(boost::alignment::aligned_alloc(
                                      kBufferAlignment, AlignUp(static_cast<std::size_t>(size_), kBufferAlignment))),
                              BoostAlignedFree{}) {}

V4L2FrameBuffer::~V4L2FrameBuffer() {}

int V4L2FrameBuffer::width() const { return width_; }

int V4L2FrameBuffer::height() const { return height_; }

uint32_t V4L2FrameBuffer::format() const { return format_; }

unsigned int V4L2FrameBuffer::size() const { return size_; }

unsigned int V4L2FrameBuffer::flags() const { return flags_; }

timeval V4L2FrameBuffer::timestamp() const { return timestamp_; }

std::shared_ptr<I420Buffer> V4L2FrameBuffer::ToI420() {
  std::shared_ptr<I420Buffer> i420_buffer(I420Buffer::Create(width_, height_, kBufferAlignment));

  if (format_ == V4L2_PIX_FMT_YUV420) {
    memcpy(i420_buffer->MutableDataY(), is_buffer_copied ? data_.get() : (uint8_t *) buffer_.start, size_);
  } else if (format_ == V4L2_PIX_FMT_H264) {
    // use hw decoded frame from track.
  } else {
    if (libyuv::ConvertToI420(is_buffer_copied ? data_.get() : (uint8_t *) buffer_.start, size_,
                              i420_buffer.get()->MutableDataY(), i420_buffer.get()->StrideY(),
                              i420_buffer.get()->MutableDataU(), i420_buffer.get()->StrideU(),
                              i420_buffer.get()->MutableDataV(), i420_buffer.get()->StrideV(), 0, 0, width_, height_,
                              width_, height_, libyuv::kRotate0, format_) < 0) {
      ERROR_PRINT("Mjpeg ConvertToI420 Failed");
    }
  }

  return i420_buffer;
}

void V4L2FrameBuffer::CopyBufferData() {
  memcpy(data_.get(), (uint8_t *) buffer_.start, size_);
  is_buffer_copied = true;
}

V4L2Buffer V4L2FrameBuffer::GetRawBuffer() { return buffer_; }

const void *V4L2FrameBuffer::Data() const { return data_.get(); }
