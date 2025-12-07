cmake_minimum_required(VERSION 3.16)
project(MiniTradingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)
add_subdirectory(api)
add_subdirectory(replay)
