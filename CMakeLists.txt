cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(nanocube2
  VERSION 0.0.1
  DESCRIPTION "Nanocubes, but simpler"
  LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)

add_executable(test_nanocube src/test_nanocube.cc src/naivecube.cc src/nanocube.cc src/debug.cc)

target_include_directories(test_nanocube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
