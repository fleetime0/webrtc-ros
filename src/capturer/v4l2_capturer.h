#ifndef V4L2_CAPTURER_H_
#define V4L2_CAPTURER_H_

#include <atomic>
#include <thread>

#include "args.h"
#include "capturer/video_capturer.h"
#include "common/interface/subject.h"
#include "common/v4l2_frame_buffer.h"
#include "common/v4l2_utils.h"

class V4L2Capturer : public VideoCapturer {
public:
  static std::shared_ptr<V4L2Capturer> Create(Args args);

  V4L2Capturer(Args args);
  ~V4L2Capturer();
  int fps() const override;
  int width() const override;
  int height() const override;
  uint32_t format() const override;
  Args config() const override;
  void StartCapture() override;

  V4L2Capturer &SetControls(int key, int value) override;

private:
  int fd_;
  int fps_;
  int width_;
  int height_;
  int buffer_count_;
  uint32_t format_;
  Args config_;

  V4L2BufferGroup capture_;
  std::atomic<bool> capture_stop_;
  std::thread capture_thread_;
  std::shared_ptr<V4L2FrameBuffer> frame_buffer_;

  V4L2Capturer &SetResolution(int width, int height) override;
  V4L2Capturer &SetFps(int fps) override;
  V4L2Capturer &SetRotation(int angle) override;

  void Init(int deviceId);
  bool IsCompressedFormat() const;
  void CaptureImage();
  void NextBuffer(V4L2Buffer &buffer);
};

#endif
