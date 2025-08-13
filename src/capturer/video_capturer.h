#ifndef VIDEO_CAPTURER_H_
#define VIDEO_CAPTURER_H_

#include "args.h"
#include "common/interface/subject.h"
#include "common/v4l2_frame_buffer.h"
#include "common/v4l2_utils.h"

#include <memory>
#include <variant>

class VideoCapturer {
public:
  VideoCapturer() = default;
  ~VideoCapturer() { frame_buffer_subject_.UnSubscribe(); }

  virtual int fps() const = 0;
  virtual int width() const = 0;
  virtual int height() const = 0;
  virtual uint32_t format() const = 0;
  virtual Args config() const = 0;
  virtual void StartCapture() = 0;

  virtual VideoCapturer &SetResolution(int width, int height) = 0;
  virtual VideoCapturer &SetFps(int fps) = 0;
  virtual VideoCapturer &SetRotation(int angle) = 0;
  virtual VideoCapturer &SetControls(int key, int value) = 0;

  std::shared_ptr<Observable<std::shared_ptr<V4L2FrameBuffer>>> AsFrameBufferObservable() {
    return frame_buffer_subject_.AsObservable();
  }

protected:
  void NextFrameBuffer(std::shared_ptr<V4L2FrameBuffer> frame_buffer) { frame_buffer_subject_.Next(frame_buffer); }

private:
  Subject<std::shared_ptr<V4L2FrameBuffer>> frame_buffer_subject_;
};

#endif
