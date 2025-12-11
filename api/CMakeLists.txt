project(API)

file(GLOB API_SRC src/*.cpp)
file(GLOB API_HEADERS include/*.hpp)

# ensure we add the new source
add_executable(api_server ${API_SRC} ${API_HEADERS})

target_include_directories(api_server PUBLIC include ../engine/include)
target_link_libraries(api_server engine sqlite3 pthread)

target_include_directories(api_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
