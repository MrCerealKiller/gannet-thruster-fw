cmake_minimum_required(VERSION 2.8.3)
project(thruster_firmware)

find_package(catkin REQUIRED COMPONENTS
  message_generation
  roscpp
  rosserial_arduino
  rosserial_client
)


###################################
## catkin specific configuration ##
###################################

catkin_package(
  CATKIN_DEPENDS
)

###########
## Build ##
###########

rosserial_generate_ros_lib(
  PACKAGE rosserial_arduino
  SCRIPT make_libraries.py
)

rosserial_configure_client(
  DIRECTORY src
  TOOLCHAIN_FILE ${ROSSERIAL_ARDUINO_TOOLCHAIN}
)

rosserial_add_client_target(src effort ALL)
rosserial_add_client_target(src effort-upload)

#############
## Install ##
#############

# Rosserial will install things automagically

