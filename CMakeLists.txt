cmake_minimum_required(VERSION 3.20)

project(GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GameEngine
    src/main.cpp
    src/Core/Engine.cpp
    src/Time/Clock.cpp
)