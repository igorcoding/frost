set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++14")
include_directories(..)

add_executable(server server.cc)
target_link_libraries(server frost)

add_executable(fork fork.cc)
target_link_libraries(fork frost)