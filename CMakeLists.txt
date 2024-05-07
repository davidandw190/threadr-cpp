cmake_minimum_required(VERSION 3.13)

project(threadr-cpp CXX)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")


add_subdirectory(src)
add_subdirectory(test)

add_custom_target(run
    COMMAND ./src/threadr
    DEPENDS threadr
)

add_custom_target(cleanup
    COMMAND ${CMAKE_COMMAND} -E remove threadr
    COMMAND ${CMAKE_COMMAND} -E remove results.txt
)