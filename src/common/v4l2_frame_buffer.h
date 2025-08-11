#ifndef V4L2_FRAME_BUFFER_H_
#define V4L2_FRAME_BUFFER_H_

#include "common/v4l2_utils.h"

#include <linux/videodev2.h>
#include <memory>
#include <vector>

class V4L2FrameBuffer {
public:
  static std::shared_ptr<V4L2FrameBuffer> Create(int width, int height, int size, uint32_t format);
  static std::shared_ptr<V4L2FrameBuffer> Create(int width, int height, V4L2Buffer buffer);

  int width() const;
  int height() const;
  //   rtc::scoped_refptr<webrtc::I420BufferInterface> ToI420();

  uint32_t format() const;
  unsigned int size() const;
  unsigned int flags() const;
  timeval timestamp() const;

  void CopyBufferData();
  const void *Data() const;
  V4L2Buffer GetRawBuffer();

protected:
  V4L2FrameBuffer(int width, int height, int size, uint32_t format);
  V4L2FrameBuffer(int width, int height, V4L2Buffer buffer);
  ~V4L2FrameBuffer();

private:
  const int width_;
  const int height_;
  const uint32_t format_;
  unsigned int size_;
  unsigned int flags_;
  bool is_buffer_copied;
  timeval timestamp_;
  V4L2Buffer buffer_;
  
  //   const std::unique_ptr<uint8_t, webrtc::AlignedFreeDeleter> data_;
};

#endif // V4L2_FRAME_BUFFER_H_
