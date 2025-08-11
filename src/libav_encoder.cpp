/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Raspberry Pi Ltd
 *
 * libav_encoder.cpp - libav video encoder.
 */

// #include <fcntl.h>
// #include <poll.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <sys/mman.h>

// // #include <libdrm/drm_fourcc.h>
// #include <linux/videodev2.h>

// #include <chrono>
#include <iostream>

#include "libav_encoder.hpp"

namespace {

void encoderOptionsGeneral(Args args, AVCodecContext *codec) {
  codec->framerate = av_d2q(args.fps, 1000);
  codec->profile = FF_PROFILE_UNKNOWN;

  std::string h264_profile = "baseline";
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

  codec->bit_rate = (int64_t) args.width * args.height * args.fps * 0.1;
}

void encoderOptionsLibx264(Args args, AVCodecContext *codec) {
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

LibAvEncoder::LibAvEncoder(Args args) : config_(args), in_fmt_ctx_(nullptr), out_fmt_ctx_(nullptr) {
  avdevice_register_all();

  av_log_set_level(AV_LOG_INFO);

  initVideoCodec();

  av_dump_format(out_fmt_ctx_, 0, nullptr, 1);

  
  // LOG(2, "libav: codec init completed");
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
  codec_ctx_[Video]->sw_pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx_[Video]->pix_fmt = AV_PIX_FMT_YUV420P;

  // Apply specific options.
  encoderOptionsLibx264(config_, codec_ctx_[Video]);

  // Apply general options.
  encoderOptionsGeneral(config_, codec_ctx_[Video]);

  // usec timebase
  codec_ctx_[Video]->time_base = av_inv_q(codec_ctx_[Video]->framerate);

  // Setup an appropriate stream/container format.
  const char *format = "rtp";

  avformat_alloc_output_context2(&out_fmt_ctx_, nullptr, format, nullptr);
  if (!out_fmt_ctx_)
    throw std::runtime_error("libav: cannot allocate output context");

  if (out_fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
    codec_ctx_[Video]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  int ret = avcodec_open2(codec_ctx_[Video], codec, nullptr);
  if (ret < 0)
    throw std::runtime_error("libav: unable to open video codec: " + std::to_string(ret));

  stream_[Video] = avformat_new_stream(out_fmt_ctx_, codec);
  if (!stream_[Video])
    throw std::runtime_error("libav: cannot allocate stream for vidout output context");

  stream_[Video]->time_base = AVRational{1, 90000};

  stream_[Video]->avg_frame_rate = stream_[Video]->r_frame_rate = codec_ctx_[Video]->framerate;
  avcodec_parameters_from_context(stream_[Video]->codecpar, codec_ctx_[Video]);
}
