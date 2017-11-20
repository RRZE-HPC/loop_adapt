cmake_minimum_required(VERSION 2.8)

# project name
project(loopAdapt_example CXX C)

set(CMAKE_PREFIX_PATH ${loopAdapt_DIR})
# find RACE
find_package(loopAdapt REQUIRED CONFIG)
message(STATUS "Found Loop-Adapt")

include_directories(${loopAdapt_INCLUDE_DIRS})
link_libraries(${loopAdapt_LIBRARIES})

include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${loopAdapt_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${loopAdapt_CXX_FLAGS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -xHOST")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -xHOST -std=c++11")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wno-write-strings")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-write-strings -std=c++11")
endif()

add_executable(LA_example_thread_scope src/jacobi_with_blocking_thread_scope.cpp src/Array.cpp)
add_executable(LA_example_mc_scope src/jacobi_with_blocking_mc_scope.cpp src/Array.cpp)
add_executable(LA_example_old src/jacobi-2D-5pt-block.c)
