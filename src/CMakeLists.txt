set(SOURCES 
    crawler.cpp
    parser.cpp
    socket.cpp
)

add_executable(threadr ${SOURCES})

target_include_directories(threadr PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(threadr argparse)

install(TARGETS threadr DESTINATION executable PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)