#include "parser.h"
#include "rtc/rtc_peer.h"

#include <algorithm>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

namespace bpo = boost::program_options;

static const std::unordered_map<std::string, int> v4l2_fmt_table = {
        {"mjpeg", V4L2_PIX_FMT_MJPEG},
        {"h264", V4L2_PIX_FMT_H264},
        {"i420", V4L2_PIX_FMT_YUV420},
        {"yuyv", V4L2_PIX_FMT_YUYV},
};

inline int ParseEnum(const std::unordered_map<std::string, int> table, const std::string &str) {
  auto it = table.find(str);
  if (it == table.end()) {
    throw std::invalid_argument("Invalid enum string: " + str);
  }
  return it->second;
}

void Parser::ParseArgs(int argc, char *argv[], Args &args) {
  bpo::options_description opts("Options");

  // clang-format off
    opts.add_options()
        ("help,h", "Display the help message")
        ("camera", bpo::value<std::string>(&args.camera)->default_value(args.camera),
            "Specify the camera using V4L2. "
            "e.g. \"v4l2:0\" for V4L2 at `/dev/video0`.")
        ("v4l2-format", bpo::value<std::string>(&args.v4l2_format)->default_value(args.v4l2_format),
            "The input format (`i420`, `yuyv`, `mjpeg`, `h264`) of the V4L2 camera.")
        ("fps", bpo::value<int>(&args.fps)->default_value(args.fps), "Specify the camera frames per second.")
        ("width", bpo::value<int>(&args.width)->default_value(args.width), "Set camera frame width.")
        ("height", bpo::value<int>(&args.height)->default_value(args.height), "Set camera frame height.")
        ("rotation", bpo::value<int>(&args.rotation)->default_value(args.rotation),
            "Set the rotation angle of the camera (0, 90, 180, 270).")
		("bitrate", bpo::value<int>(&args.bitrate)->default_value(args.bitrate),
			"Set the video bitrate for encoding.")
        ("peer-timeout", bpo::value<int>(&args.peer_timeout)->default_value(args.peer_timeout),
            "The connection timeout (in seconds) after receiving a remote offer")
        ("stun-url", bpo::value<std::string>(&args.stun_url)->default_value(args.stun_url),
            "Set the STUN server URL for WebRTC. e.g. `stun:xxx.xxx.xxx`.")
        ("http-port", bpo::value<uint16_t>(&args.http_port)->default_value(args.http_port),
            "Local HTTP server port to handle signaling when using WHEP.");
  // clang-format on

  bpo::variables_map vm;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
    bpo::notify(vm);
  } catch (const bpo::error &ex) {
    std::cerr << "Error parsing arguments: " << ex.what() << std::endl;
    exit(1);
  }

  if (vm.count("help")) {
    std::cout << opts << std::endl;
    exit(1);
  }

  if (!args.stun_url.empty() && args.stun_url.substr(0, 4) != "stun") {
    std::cout << "Stun url should not be empty and start with \"stun:\"" << std::endl;
    exit(1);
  }

  ParseDevice(args);
}

void Parser::ParseDevice(Args &args) {
  size_t pos = args.camera.find(':');
  if (pos == std::string::npos) {
    throw std::runtime_error("Unknown device format: " + args.camera);
  }

  std::string prefix = args.camera.substr(0, pos);
  std::string id = args.camera.substr(pos + 1);

  try {
    args.cameraId = std::stoi(id);
  } catch (const std::exception &e) {
    throw std::runtime_error("Invalid camera ID: " + id);
  }

  if (prefix == "v4l2") {
    args.format = ParseEnum(v4l2_fmt_table, args.v4l2_format);
    std::cout << "Using V4L2, ID: " << args.cameraId << std::endl;
    std::cout << "Using V4L2, format: " << args.v4l2_format << std::endl;
  } else {
    throw std::runtime_error("Unknown device format: " + prefix);
  }
}
