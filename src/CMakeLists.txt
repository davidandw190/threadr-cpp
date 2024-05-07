set(SOURCES 
    crawler.cpp
)

add_executable(threadr ${SOURCES})

target_include_directories(threadr PUBLIC ${CMAKE_SOURCE_DIR}/include)

install(TARGETS threadr DESTINATION executable PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)