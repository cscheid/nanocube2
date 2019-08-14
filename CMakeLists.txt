cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(nanocube2
  VERSION 0.0.1
  DESCRIPTION "Nanocubes, but simpler"
  LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)

################################################################################

add_executable(test_garbagecube
  src/garbagecube.cc src/test_garbagecube.cc src/naivecube.cc src/debug.cc src/utils.cc)

target_include_directories(test_garbagecube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_satgarbagecube
  lib/immutable/array.cc
  src/satgarbagecube.cc src/test_satgarbagecube.cc src/naivecube.cc src/debug.cc src/utils.cc)

target_include_directories(test_satgarbagecube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)



