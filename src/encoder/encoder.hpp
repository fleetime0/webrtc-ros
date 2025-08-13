/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * encoder.hpp - Video encoder class.
 */

#pragma once

#include "capturer/video_capturer.h"
#include "common/h264_frame_buffer.h"

class Encoder {
public:
  Encoder() = default;
  virtual ~Encoder() {
    video_observer_.reset();
    frame_buffer_subject_.UnSubscribe();
  }

  std::shared_ptr<Observable<std::shared_ptr<H264FrameBuffer>>> AsFrameBufferObservable() {
    return frame_buffer_subject_.AsObservable();
  }

protected:
  virtual void EncodeBuffer(std::shared_ptr<V4L2FrameBuffer> buffer) = 0;

  virtual void SubscribeVideoSource(std::shared_ptr<VideoCapturer> video_src) = 0;

  void NextFrameBuffer(std::shared_ptr<H264FrameBuffer> frame_buffer) { frame_buffer_subject_.Next(frame_buffer); }

  std::shared_ptr<Observable<std::shared_ptr<V4L2FrameBuffer>>> video_observer_;

private:
  Subject<std::shared_ptr<H264FrameBuffer>> frame_buffer_subject_;
};
