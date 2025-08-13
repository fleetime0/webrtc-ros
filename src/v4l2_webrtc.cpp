#include "v4l2_webrtc.h"

#include "capturer/v4l2_capturer.h"
#include "encoder/libav_encoder.hpp"

std::shared_ptr<V4L2Webrtc> V4L2Webrtc::Create(Args args) { return std::make_shared<V4L2Webrtc>(args); }

V4L2Webrtc::V4L2Webrtc(Args args) : args_(args) {
  video_capture_ = V4L2Capturer::Create(args);
  encoder_ = LibAvEncoder::Create(video_capture_, args);
}

Args V4L2Webrtc::config() const { return args_; }


std::shared_ptr<RtcPeer> V4L2Webrtc::CreatePeerConnection(PeerConfig peer_config) {
  auto peer = RtcPeer::Create(encoder_, peer_config);
  return peer;
}
