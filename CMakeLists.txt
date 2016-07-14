cmake_minimum_required (VERSION 2.6)
project (main)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(main ref_counted_vec.cc nanocube_traversals.cc main.cc debug.cc)
