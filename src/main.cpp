#include <capturer/v4l2_capturer.h>
#include <encoder/libav_encoder.hpp>
#include <iostream>
#include <thread>


int main() {
  Args arg;
  auto capturer = V4L2Capturer::Create(arg);
  auto observer = capturer->AsFrameBufferObservable();
  auto encoder = LibAvEncoder::Create(capturer, arg);
  // observer->Subscribe([](std::shared_ptr<V4L2FrameBuffer> frame_buffer) {
  //   std::cout << "111111111111" << std::endl;
  // });
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "Hello World!" << std::endl;
}
