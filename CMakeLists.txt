cmake_minimum_required(VERSION 3.18)
project(webrtc-ros)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system thread)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

add_subdirectory(deps/libyuv)
add_subdirectory(deps/libdatachannel)

add_executable(${PROJECT_NAME}
  src/main.cpp
  # src/signaling/http_service.cpp
  src/v4l2_webrtc.cpp
  src/rtc/rtc_peer.cpp
  src/common/v4l2_utils.cpp
  src/common/logging.cpp
  src/common/h264_frame_buffer.cpp
  src/common/v4l2_frame_buffer.cpp
  src/capturer/v4l2_capturer.cpp
  src/encoder/libav_encoder.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  ${AVCODEC_INCLUDE_DIRS}
  ${AVUTIL_INCLUDE_DIRS}
)

# target_compile_features(${PROJECT_NAME} PUBLIC c_std_99 cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  Boost::system
  Boost::thread
  LibDataChannel::LibDataChannel
  Threads::Threads
  yuv
  JPEG::JPEG
  ${AVCODEC_LIBRARIES}
  ${AVUTIL_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE=1)