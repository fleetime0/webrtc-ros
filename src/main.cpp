#include "parser.h"
#include "signaling/http_service.h"
#include "v4l2_webrtc.h"

int main(int argc, char *argv[]) {
  Args args;
  Parser::ParseArgs(argc, argv, args);
  auto v4l2_webrtc = V4L2Webrtc::Create(args);

  boost::asio::io_context ioc;
  auto http_service = HttpService::Create(args, v4l2_webrtc, ioc);
  http_service->Start();

  ioc.run();
}
