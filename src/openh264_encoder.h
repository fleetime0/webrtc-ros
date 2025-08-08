#ifndef OPENH264_ENCODER_
#define OPENH264_ENCODER_

#include <functional>

#include <wels/codec_api.h>

class Openh264Encoder {
  public:
  //   static std::unique_ptr<Openh264Encoder> Create(Args args);
  //   Openh264Encoder(Args args);
  ~Openh264Encoder();
  void Init();
  //   void Encode(rtc::scoped_refptr<webrtc::I420BufferInterface> frame_buffer,
  //               std::function<void(uint8_t *, int)> on_capture);

  private:
  int fps_;
  int width_;
  int height_;
  int bitrate_;
  ISVCEncoder *encoder_;
  SSourcePicture src_pic_;
};

#endif // OPENH264_ENCODER_
