/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Raspberry Pi Ltd
 *
 * libav_encoder.cpp - libav video encoder.
 */

#include <iostream>

#include "common/logging.h"
#include "libav_encoder.hpp"

namespace {

void encoderOptionsGeneral(Args args, AVCodecContext *codec) {
  codec->framerate = {args.fps * 1000, 1000};
  codec->profile = FF_PROFILE_UNKNOWN;

  std::string h264_profile = "constrained baseline";
  const AVCodecDescriptor *desc = avcodec_descriptor_get(codec->codec_id);
  for (const AVProfile *profile = desc->profiles; profile && profile->profile != FF_PROFILE_UNKNOWN; profile++) {
    if (!strncasecmp(h264_profile.c_str(), profile->name, h264_profile.size())) {
      codec->profile = profile->profile;
      break;
    }
  }
  if (codec->profile == FF_PROFILE_UNKNOWN)
    throw std::runtime_error("libav: no such profile " + h264_profile);

  codec->level = FF_LEVEL_UNKNOWN;
  codec->gop_size = args.fps;

  codec->bit_rate = args.bitrate * 1000;
}

void encoderOptionsLibx264([[maybe_unused]] Args args, AVCodecContext *codec) {
  codec->me_range = 16;
  codec->me_cmp = 1; // No chroma ME
  codec->me_subpel_quality = 0;
  codec->thread_count = 0;

  codec->thread_type = FF_THREAD_SLICE;
  codec->slices = 4;
  codec->refs = 1;
  av_opt_set(codec->priv_data, "preset", "ultrafast", 0);
  av_opt_set(codec->priv_data, "tune", "zerolatency", 0);

  av_opt_set(codec->priv_data, "weightp", "none", 0);
  av_opt_set(codec->priv_data, "weightb", "0", 0);
  av_opt_set(codec->priv_data, "motion-est", "dia", 0);
  av_opt_set(codec->priv_data, "sc_threshold", "0", 0);
  av_opt_set(codec->priv_data, "rc-lookahead", "0", 0);
  av_opt_set(codec->priv_data, "mixed_ref", "0", 0);
}

} // namespace

std::shared_ptr<LibAvEncoder> LibAvEncoder::Create(std::shared_ptr<VideoCapturer> video_src, Args args) {
  auto ptr = std::make_shared<LibAvEncoder>(args);
  ptr->SubscribeVideoSource(video_src);

  return ptr;
}

LibAvEncoder::LibAvEncoder(Args args) : config_(args), video_start_ts_(0) {
  av_log_set_level(AV_LOG_INFO);

  initVideoCodec();

  pkt_[Video] = av_packet_alloc();
  DEBUG_PRINT("libav: codec init completed");
}

LibAvEncoder::~LibAvEncoder() {
  avcodec_free_context(&codec_ctx_[Video]);

  av_packet_free(&pkt_[Video]);
  DEBUG_PRINT("libav: codec closed");
}

void LibAvEncoder::EncodeBuffer(std::shared_ptr<V4L2FrameBuffer> buffer) {
  auto t_start = std::chrono::high_resolution_clock::now();
  AVFrame *frame = av_frame_alloc();
  if (!frame)
    throw std::runtime_error("libav: could not allocate AVFrame");

  auto tv_to_us = [](const timeval &tv) { return static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec; };

  if (!video_start_ts_) {
    video_start_ts_ = tv_to_us(buffer->timestamp());

    DEBUG_PRINT("Video start timestamp : %" PRId64 "", video_start_ts_);
  }

  auto i420_buffer = buffer->ToI420();

  frame->format = codec_ctx_[Video]->pix_fmt;
  frame->width = i420_buffer->width();
  frame->height = i420_buffer->height();

  frame->linesize[0] = i420_buffer->StrideY();
  frame->linesize[1] = i420_buffer->StrideU();
  frame->linesize[2] = i420_buffer->StrideV();

  const int64_t ts_us = tv_to_us(buffer->timestamp());
  frame->pts = ts_us - video_start_ts_;

  auto *holder = new std::shared_ptr<I420Buffer>(i420_buffer);

  frame->buf[0] = av_buffer_create(i420_buffer->MutableDataY(), i420_buffer->ByteSize(), &LibAvEncoder::releaseBuffer,
                                   holder, 0);
  av_image_fill_pointers(frame->data, AV_PIX_FMT_YUV420P, frame->height, frame->buf[0]->data, frame->linesize);
  av_frame_make_writable(frame);

  int ret = avcodec_send_frame(codec_ctx_[Video], frame);
  if (ret < 0)
    throw std::runtime_error("libav: error encoding frame: " + std::to_string(ret));

  encode(pkt_[Video], Video);

  av_frame_free(&frame);

  // 记录结束时间
  auto t_end = std::chrono::high_resolution_clock::now();
  double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

  // 打印单次耗时
  std::cout << "[media] EncodeBuffer耗时: " << elapsed_ms << " ms" << std::endl;
}

void LibAvEncoder::SubscribeVideoSource(std::shared_ptr<VideoCapturer> video_src) {
  video_observer_ = video_src->AsFrameBufferObservable();
  video_observer_->Subscribe([this](std::shared_ptr<V4L2FrameBuffer> buffer) { EncodeBuffer(buffer); });
}

void LibAvEncoder::initVideoCodec() {
  const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
  if (!codec)
    throw std::runtime_error("libav: cannot find video encoder libx264");

  codec_ctx_[Video] = avcodec_alloc_context3(codec);
  if (!codec_ctx_[Video])
    throw std::runtime_error("libav: Cannot allocate video context");

  codec_ctx_[Video]->width = config_.width;
  codec_ctx_[Video]->height = config_.height;
  // usec timebase
  codec_ctx_[Video]->time_base = {1, 1000 * 1000};
  codec_ctx_[Video]->sw_pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx_[Video]->pix_fmt = AV_PIX_FMT_YUV420P;

  // Apply specific options.
  encoderOptionsLibx264(config_, codec_ctx_[Video]);

  // Apply general options.
  encoderOptionsGeneral(config_, codec_ctx_[Video]);

  int ret = avcodec_open2(codec_ctx_[Video], codec, nullptr);
  if (ret < 0)
    throw std::runtime_error("libav: unable to open video codec: " + std::to_string(ret));
}

void LibAvEncoder::encode(AVPacket *pkt, unsigned int stream_id) {
  int ret = 0;

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_ctx_[stream_id], pkt);

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      av_packet_unref(pkt);
      break;
    } else if (ret < 0)
      throw std::runtime_error("libav: error receiving packet: " + std::to_string(ret));

    bool key = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
    auto frame_buffer = H264FrameBuffer::Create(pkt->data, pkt->size, key, pkt->pts);
    NextFrameBuffer(frame_buffer);

    av_packet_unref(pkt);
  }
}

extern "C" void LibAvEncoder::releaseBuffer(void *opaque, uint8_t *) {
  auto *p = static_cast<std::shared_ptr<I420Buffer> *>(opaque);
  delete p;
}
