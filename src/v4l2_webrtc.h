#ifndef V4L2_WEBRTC_H
#define V4L2_WEBRTC_H

#include <memory>

#include "args.h"
#include "capturer/video_capturer.h"
#include "encoder/encoder.hpp"
#include "rtc/rtc_peer.h"

class V4L2Webrtc {
public:
  static std::shared_ptr<V4L2Webrtc> Create(Args args);

  V4L2Webrtc(Args args);
  ~V4L2Webrtc() = default;

  Args config() const;
  std::shared_ptr<RtcPeer> CreatePeerConnection(PeerConfig peer_config);

private:
  Args args_;

  std::shared_ptr<VideoCapturer> video_capture_;
  std::shared_ptr<Encoder> encoder_;
};

#endif // V4L2_WEBRTC_H
