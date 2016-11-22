cmake_minimum_required (VERSION 2.6)
project (nanocubes)

find_package(Boost 1.36.0 COMPONENTS date_time)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(THIRDPARTY_FILES ./src/mongoose.c)

set(NANOCUBE_FILES
  ./src/nanocube_traversals.cc
  ./src/ref_counted_vec.cc
  ./src/debug.cc
)

set(NAIVECUBE_FILES
  ./src/naivecube.h
  ./src/naivecube.inc
)

add_executable(ncserver ${NANOCUBE_FILES} ${THIRDPARTY_FILES} ./src/ncserver.cc)
add_executable(naivecubeserver ${NANOCUBE_FILES} ${NAIVECUBE_FILES} ${THIRDPARTY_FILES} ./src/naivecube_server.cc)

target_link_libraries(ncserver ${Boost_LIBRARIES})
target_link_libraries(naivecubeserver ${Boost_LIBRARIES})

set(CMAKE_BUILD_TYPE Release)
