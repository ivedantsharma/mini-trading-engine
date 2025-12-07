project(Engine)

file(GLOB ENGINE_SRC src/*.cpp)
file(GLOB ENGINE_HEADERS include/*.hpp)

add_library(engine STATIC ${ENGINE_SRC} ${ENGINE_HEADERS})
target_include_directories(engine PUBLIC include)

add_executable(engine_runner src/engine_main.cpp)
target_link_libraries(engine_runner PRIVATE engine)
target_include_directories(engine_runner PRIVATE include)
