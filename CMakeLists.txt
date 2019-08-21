cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(nanocube2
  VERSION 0.0.1
  DESCRIPTION "Nanocubes, but simpler"
  LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
find_package(Boost COMPONENTS thread chrono regex REQUIRED)

################################################################################

add_executable(test_persistentsat
  lib/immutable/array.cc
  src/test_persistentsat.cc src/persistentsat.cc src/debug.cc src/utils.cc)

target_include_directories(test_persistentsat PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_sparsesat
  src/test_sparsesat.cc src/sparsesat.cc src/debug.cc src/utils.cc)

target_include_directories(test_sparsesat PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_garbagecube
  lib/immutable/array.cc
  src/garbagecube.cc src/persistentsat.cc src/test_garbagecube.cc src/naivecube.cc src/debug.cc src/utils.cc)

target_include_directories(test_garbagecube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_satgarbagecube
  lib/immutable/array.cc
  src/satgarbagecube.cc src/test_satgarbagecube.cc src/naivecube.cc src/debug.cc src/utils.cc)

target_include_directories(test_satgarbagecube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_nanocube
  lib/immutable/array.cc
  src/nanocube.cc src/test_nanocube.cc src/naivecube.cc src/debug.cc src/utils.cc)

target_include_directories(test_nanocube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_persistentsatcube
  lib/immutable/array.cc
  src/garbagecube.cc src/persistentsatcube.cc src/test_persistentsatcube.cc
  src/naivecube.cc src/nanocube.cc src/debug.cc src/utils.cc)

target_include_directories(test_persistentsatcube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_sparsesatcube
  src/garbagecube.cc src/sparsesatcube.cc src/test_sparsesatcube.cc
  src/naivecube.cc src/nanocube.cc src/debug.cc src/utils.cc)

target_include_directories(test_sparsesatcube PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

################################################################################

add_executable(test_nyc_pickup
  lib/immutable/array.cc
  src/garbagecube.cc src/nanocube.cc
  src/sparsesat.cc src/persistentsat.cc
  src/test_nyc_pickup.cc src/debug.cc src/utils.cc)

target_include_directories(test_nyc_pickup PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(test_nyc_pickup ${Boost_LIBRARIES})

