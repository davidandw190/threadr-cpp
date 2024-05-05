set(SOURCES 
    crawler.cpp
)

add_executable(crawler ${SOURCES})

target_include_directories(crawler PUBLIC ${CMAKE_SOURCE_DIR}/include)

install(TARGETS crawler DESTINATION executable PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)