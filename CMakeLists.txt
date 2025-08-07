cmake_minimum_required(VERSION 3.13)
project(webrtc-ros)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS system thread)

add_subdirectory(deps/libdatachannel)

add_executable(${PROJECT_NAME}
  src/main.cpp
  # src/http_service.cpp
  # src/webrtc_ros.cpp
  src/rtc_peer.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_compile_features(${PROJECT_NAME} PUBLIC c_std_99 cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  Boost::system
  Boost::thread
  LibDataChannel::LibDataChannel
  Threads::Threads
)
