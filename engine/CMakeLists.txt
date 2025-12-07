project(Engine)

file(GLOB ENGINE_SRC src/*.cpp)
file(GLOB ENGINE_HEADERS include/*.hpp)

add_library(engine STATIC ${ENGINE_SRC} ${ENGINE_HEADERS})

target_include_directories(engine PUBLIC include)
