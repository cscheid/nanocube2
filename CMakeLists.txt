cmake_minimum_required (VERSION 2.6)
project (nanocubes)

find_package(Boost 1.36.0 COMPONENTS date_time)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(NANOCUBE_FILES
  ./src/nanocube_traversals.cc
  ./src/ref_counted_vec.cc
  ./src/debug.cc
  ./src/mongoose.c
)

set(NAIVECUBE_FILES
  ./src/naivecube.h
  ./src/naivecube.inc
)

#add_executable(main ${NANOCUBE_FILES} ./src/tests/main.cc)
#add_executable(nanocube_tests ${NANOCUBE_FILES} ./src/tests/tests.cc)
#add_executable(test_brightkite ${NANOCUBE_FILES} ./src/tests/test_brightkite.cc)
#add_executable(test_flights ${NANOCUBE_FILES} ./src/tests/test_flights.cc)
add_executable(ncserver ${NANOCUBE_FILES} ${NAIVECUBE_FILES} ./src/ncserver.cc)

#target_link_libraries(nanocube_tests ${Boost_LIBRARIES})
#target_link_libraries(test_brightkite ${Boost_LIBRARIES})
#target_link_libraries(test_flights ${Boost_LIBRARIES})
target_link_libraries(ncserver ${Boost_LIBRARIES})

set(CMAKE_BUILD_TYPE Release)
