#ifndef ARGS_H_
#define ARGS_H_

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

#include <linux/videodev2.h>

template<typename DEFAULT>
struct TimeVal {
  TimeVal() : value(0) {}

  void set(const std::string &s) {
    static const std::unordered_map<std::string, std::chrono::nanoseconds> match{
            {"min", std::chrono::minutes(1)},     {"sec", std::chrono::seconds(1)},
            {"s", std::chrono::seconds(1)},       {"ms", std::chrono::milliseconds(1)},
            {"us", std::chrono::microseconds(1)}, {"ns", std::chrono::nanoseconds(1)},
    };

    try {
      std::size_t end_pos;
      float f = std::stof(s, &end_pos);
      value = std::chrono::duration_cast<std::chrono::nanoseconds>(f * DEFAULT{1});

      for (const auto &m: match) {
        auto found = s.find(m.first, end_pos);
        if (found != end_pos || found + m.first.length() != s.length())
          continue;
        value = std::chrono::duration_cast<std::chrono::nanoseconds>(f * m.second);
        break;
      }
    } catch (std::exception const &e) {
      throw std::runtime_error("Invalid time string provided");
    }
  }

  template<typename C = DEFAULT>
  int64_t get() const {
    return std::chrono::duration_cast<C>(value).count();
  }

  explicit constexpr operator bool() const { return !!value.count(); }

  std::chrono::nanoseconds value;
};

struct Args {
  // video input
  int cameraId = 0;
  int fps = 30;
  int width = 640;
  int height = 480;
  int rotation = 0;
  uint32_t format = V4L2_PIX_FMT_MJPEG;

  // h264
  int bitrate = 1000;

  // webrtc
  int peer_timeout = 10;
  uint16_t http_port = 8080;
};

#endif // ARGS_H_
