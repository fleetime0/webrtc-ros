/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Raspberry Pi Ltd
 *
 * libav_encoder.hpp - libav video encoder.
 */

#pragma once

#include <condition_variable>
#include <memory>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
}

#include "args.h"
#include "encoder.hpp"

class LibAvEncoder : public Encoder {
public:
  static std::shared_ptr<LibAvEncoder> Create(std::shared_ptr<VideoCapturer> video_src, Args args);

  LibAvEncoder(Args args);
  ~LibAvEncoder();

protected:
  void EncodeBuffer(std::shared_ptr<V4L2FrameBuffer> buffer) override;

  void SubscribeVideoSource(std::shared_ptr<VideoCapturer> video_src) override;

private:
  void initVideoCodec();

  void encode(AVPacket *pkt, unsigned int stream_id);

  static void releaseBuffer(void *opaque, uint8_t *data);

  Args config_;

  uint64_t video_start_ts_;

  enum Context { Video = 0, Audio = 1 };
  AVCodecContext *codec_ctx_[2];

  AVPacket *pkt_[2];
};
