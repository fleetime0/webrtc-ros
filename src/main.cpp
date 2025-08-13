#include <iostream>
#include <thread>
#include <v4l2_webrtc.h>

int main() {
  Args arg;
  auto v4l2_webrtc = V4L2Webrtc::Create(arg);

  std::this_thread::sleep_for(std::chrono::seconds(60)); // 录 10 秒
  // ofs.close();
}
