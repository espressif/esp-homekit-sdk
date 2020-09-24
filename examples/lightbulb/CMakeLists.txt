
# (Automatically converted from project Makefile by convert_to_cmake.py.)

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if(DEFINED ENV{HOMEKIT_PATH})
  set(HOMEKIT_PATH $ENV{HOMEKIT_PATH})
else()
  set(HOMEKIT_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif(DEFINED ENV{HOMEKIT_PATH})

set(EXTRA_COMPONENT_DIRS ${HOMEKIT_PATH}/components ${HOMEKIT_PATH}/components/homekit ${CMAKE_CURRENT_LIST_DIR}/../common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lightbulb)
