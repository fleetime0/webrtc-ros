#include "common/h264_frame_buffer.h"

std::shared_ptr<H264FrameBuffer> H264FrameBuffer::Create(uint8_t *data, size_t size, bool keyframe, int64_t pts,
                                                         Deleter deleter) {
  return std::make_shared<H264FrameBuffer>(data, size, keyframe, pts, deleter);
}

H264FrameBuffer::H264FrameBuffer(uint8_t *data, size_t size, bool keyframe, int64_t pts, Deleter deleter) :
    data_(data, std::move(deleter)), size_(size), keyframe_(keyframe), pts_(pts) {}


const uint8_t *H264FrameBuffer::data() const { return data_.get(); }

size_t H264FrameBuffer::size() const { return size_; }

bool H264FrameBuffer::isKeyFrame() const { return keyframe_; }

int64_t H264FrameBuffer::pts() const { return pts_; }
