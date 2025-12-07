project(Replay)

file(GLOB REPLAY_SRC src/*.cpp)
file(GLOB REPLAY_HEADERS include/*.hpp)

add_executable(replay_tool ${REPLAY_SRC} ${REPLAY_HEADERS})

target_include_directories(replay_tool PUBLIC include ../engine/include)
target_link_libraries(replay_tool engine)
