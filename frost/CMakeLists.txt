if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(.)
set(SOURCE_FILES
                 http_server.h
                 http_server.cc
                 http_request.h
                 http_request.cc
                 http_response.h
                 http_response.cc
                 router.h
                 util/util.h
                 util/mapper.h
                 http/status.h
                 http/status.cc

                 )
add_library(frost ${SOURCE_FILES})
target_link_libraries(frost ev)
