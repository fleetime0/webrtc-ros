cmake_minimum_required(VERSION 3.18)
project(webrtc-ros)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system thread)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVDEVICE REQUIRED libavdevice)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
# pkg_check_modules(OPENH264 REQUIRED openh264)

add_subdirectory(deps/libdatachannel)

add_executable(${PROJECT_NAME}
  src/main.cpp
  # src/http_service.cpp
  # src/webrtc_ros.cpp
  src/rtc_peer.cpp
  src/common/v4l2_utils.cpp
  src/libav_encoder.cpp
  # src/openh264_encoder.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  ${AVCODEC_INCLUDE_DIRS}
  ${AVFORMAT_INCLUDE_DIRS}
  ${AVUTIL_INCLUDE_DIRS}
  ${AVDEVICE_INCLUDE_DIRS}
  # ${OPENH264_INCLUDE_DIRS}
)

# target_compile_features(${PROJECT_NAME} PUBLIC c_std_99 cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  Boost::system
  Boost::thread
  LibDataChannel::LibDataChannel
  Threads::Threads
  ${AVCODEC_LIBRARIES}
  ${AVFORMAT_LIBRARIES}
  ${AVUTIL_LIBRARIES}
  ${AVDEVICE_LIBRARIES}
  # ${OPENH264_LIBRARIES}
)
