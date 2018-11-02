cmake_minimum_required(VERSION 2.8.3)

###########
## Build ##
###########

include_directories(${ROS_LIB_DIR})

generate_arduino_firmware(effort
  SKETCH effort.ino
  SRCS ${ROS_LIB_DIR}/time.cpp
  BOARD uno
  PORT /dev/arduino_thrusters
)
