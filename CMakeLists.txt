cmake_minimum_required(VERSION 3.20)

project(GameEngine)
find_package(SDL3 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GameEngine
    src/main.cpp
    src/Core/Engine.cpp
    src/Time/Clock.cpp
    src/World/World.cpp
    src/World/Position.cpp
    src/Entity/Entity.cpp
    src/Entity/Manager/EntityManager.cpp
    src/Movement/Movement.cpp
    src/World/Map.cpp
    src/World/Tile/Tile.cpp
    src/World/Object/WorldObject.cpp
    src/World/Object/Resource/ResourceNode.cpp
    src/World/Object/Manager/ObjectManager.cpp
    src/Action/Action.cpp
    src/Action/ActionManager.cpp
    src/Movement/MovementRequest.cpp
    src/Input/InputManager.cpp
    src/Player/Player.cpp
    src/NPC/NPC.cpp
    src/Graphics/Graphics.cpp
)
target_include_directories(GameEngine PRIVATE src)
target_link_libraries(GameEngine PRIVATE SDL3::SDL3)
