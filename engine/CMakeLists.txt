project(Engine)

find_package(Boost REQUIRED COMPONENTS system)
find_package(SQLite3 REQUIRED)

file(GLOB ENGINE_SRC src/*.cpp)
file(GLOB ENGINE_HEADERS include/*.hpp)

add_library(engine STATIC ${ENGINE_SRC} ${ENGINE_HEADERS})
target_include_directories(engine PUBLIC include)
target_link_libraries(engine PUBLIC Boost::system SQLite::SQLite3 pthread)

add_executable(engine_runner src/engine_main.cpp)
target_link_libraries(engine_runner PRIVATE engine Boost::system SQLite::SQLite3 pthread)
