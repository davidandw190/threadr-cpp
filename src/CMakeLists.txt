set(SOURCES 
    crawler.cpp
)

add_executable(crawler ${SOURCES})

target_include_directories(crawler PUBLIC ${CMAKE_SOURCE_DIR}/include)