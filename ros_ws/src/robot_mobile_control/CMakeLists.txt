cmake_minimum_required(VERSION 3.8)

project(robot_mobile_control)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
  )
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(mobile_bridge
  src/mobile_bridge.cpp
)

ament_target_dependencies(mobile_bridge
  rclcpp
  geometry_msgs
)

target_link_libraries(mobile_bridge
  pthread
)

install(
  TARGETS
    mobile_bridge
  DESTINATION
    lib/${PROJECT_NAME}
)

install(
  DIRECTORY
    config
    description
    launch
  DESTINATION
    share/${PROJECT_NAME}
)

ament_package()