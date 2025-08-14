#ifndef ARGS_H_
#define ARGS_H_

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

#include <linux/videodev2.h>

struct Args {
  // video input
  int cameraId = 0;
  int fps = 30;
  int width = 640;
  int height = 480;
  int rotation = 0;
  uint32_t format = V4L2_PIX_FMT_MJPEG;
  std::string camera = "v4l2:0";
  std::string v4l2_format = "mjpeg";

  // h264
  int bitrate = 1000;

  // webrtc
  int peer_timeout = 10;
  uint16_t http_port = 8000;
  std::string stun_url = "stun:stun.l.google.com:19302";
};

#endif // ARGS_H_
