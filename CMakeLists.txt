cmake_minimum_required(VERSION 3.2)
project(frost)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_INCLUDE_PATH})

find_package(LibEV REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(third_party/libevserver/include/)

add_subdirectory(third_party/libevserver/)
add_subdirectory(frost/)
add_subdirectory(examples/)
