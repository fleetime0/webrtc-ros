#include "v4l2_capturer.h"

#include <cstring>

// Linux
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/select.h>

#include "common/logging.h"

std::shared_ptr<V4L2Capturer> V4L2Capturer::Create(Args args) {
  auto ptr = std::make_shared<V4L2Capturer>(args);
  ptr->Init(args.cameraId);

  ptr->SetFps(args.fps)
          .SetRotation(args.rotation)
          .SetResolution(args.width, args.height)
          .SetControls(V4L2_CID_MPEG_VIDEO_BITRATE, 10000 * 1000)
          .StartCapture();
  return ptr;
}

V4L2Capturer::V4L2Capturer(Args args) : buffer_count_(4), format_(args.format), config_(args), capture_stop_(false) {}

void V4L2Capturer::Init(int deviceId) {
  std::string devicePath = "/dev/video" + std::to_string(deviceId);
  fd_ = V4L2Util::OpenDevice(devicePath.c_str());

  if (!V4L2Util::InitBuffer(fd_, &capture_, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_MEMORY_MMAP)) {
    exit(0);
  }
}

V4L2Capturer::~V4L2Capturer() {
  capture_stop_ = true;

  if (capture_thread_.joinable()) {
    capture_thread_.join();
  }
  V4L2Util::StreamOff(fd_, capture_.type);
  V4L2Util::DeallocateBuffer(fd_, &capture_);
  V4L2Util::CloseDevice(fd_);
}

int V4L2Capturer::fps() const { return fps_; }

int V4L2Capturer::width() const { return width_; }

int V4L2Capturer::height() const { return height_; }

uint32_t V4L2Capturer::format() const { return format_; }

Args V4L2Capturer::config() const { return config_; }

bool V4L2Capturer::IsCompressedFormat() const { return format_ == V4L2_PIX_FMT_MJPEG; }

V4L2Capturer &V4L2Capturer::SetResolution(int width, int height) {
  width_ = width;
  height_ = height;
  V4L2Util::SetFormat(fd_, &capture_, width, height, format_);
  return *this;
}

V4L2Capturer &V4L2Capturer::SetFps(int fps) {
  fps_ = fps;
  DEBUG_PRINT("  Fps: %d", fps);
  if (!V4L2Util::SetFps(fd_, capture_.type, fps)) {
    exit(0);
  }
  return *this;
}

V4L2Capturer &V4L2Capturer::SetRotation(int angle) {
  DEBUG_PRINT("  Rotation: %d", angle);
  V4L2Util::SetCtrl(fd_, V4L2_CID_ROTATE, angle);
  return *this;
}

void V4L2Capturer::CaptureImage() {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd_, &fds);
  timeval tv = {};
  tv.tv_sec = 0;
  tv.tv_usec = 200000;
  int r = select(fd_ + 1, &fds, NULL, NULL, &tv);
  if (r == -1) {
    ERROR_PRINT("select failed");
    return;
  } else if (r == 0) { // timeout
    DEBUG_PRINT("capture timeout");
    return;
  }

  v4l2_buffer buf = {};
  buf.type = capture_.type;
  buf.memory = capture_.memory;

  if (!V4L2Util::DequeueBuffer(fd_, &buf)) {
    return;
  }

  auto buffer = V4L2Buffer::FromV4L2((uint8_t *) capture_.buffers[buf.index].start, buf, format_);
  NextBuffer(buffer);

  if (!V4L2Util::QueueBuffer(fd_, &buf)) {
    return;
  }
}

V4L2Capturer &V4L2Capturer::SetControls(int key, int value) {
  V4L2Util::SetExtCtrl(fd_, key, value);
  return *this;
}

void V4L2Capturer::NextBuffer(V4L2Buffer &buffer) {
  frame_buffer_ = V4L2FrameBuffer::Create(width_, height_, buffer);
  NextFrameBuffer(frame_buffer_);
}

void V4L2Capturer::StartCapture() {
  if (!V4L2Util::AllocateBuffer(fd_, &capture_, buffer_count_) || !V4L2Util::QueueBuffers(fd_, &capture_)) {
    exit(0);
  }

  V4L2Util::StreamOn(fd_, capture_.type);
  capture_thread_ = std::thread([this]() {
    while (!capture_stop_) {
      CaptureImage();
    }
  });
}
