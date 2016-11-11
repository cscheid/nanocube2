cmake_minimum_required (VERSION 2.6)
project (main)

find_package(Boost 1.36.0 COMPONENTS date_time)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(NANOCUBE_FILES ref_counted_vec.cc nanocube_traversals.cc debug.cc)
set(MONGOOSE_FILES mongoose.c)

# add_executable(main ${NANOCUBE_FILES} main.cc)
# add_executable(nanocube_tests ${NANOCUBE_FILES} tests.cc)
#add_executable(test_brightkite ${NANOCUBE_FILES} test_brightkite.cc)
#add_executable(test_flights ${NANOCUBE_FILES} test_flights.cc)
add_executable(ncserver ${MONGOOSE_FILES} ${NANOCUBE_FILES} ncserver.cc)

#target_link_libraries(test_brightkite ${Boost_LIBRARIES})
#target_link_libraries(test_flights ${Boost_LIBRARIES})
target_link_libraries(ncserver ${Boost_LIBRARIES})


set(CMAKE_BUILD_TYPE Release)
