cmake_minimum_required(VERSION 3.20)

project(GameEngine)
find_package(SDL3 REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAMEENGINE_CORE_SOURCES
    src/Stats/StatBlock.cpp
    src/World/World.cpp
    src/World/Position.cpp
    src/Entity/Entity.cpp
    src/Entity/Manager/EntityManager.cpp
    src/Movement/Movement.cpp
    src/World/Map.cpp
    src/World/Tile/Tile.cpp
    src/World/Object/WorldObject.cpp
    src/World/Object/Resource/ResourceNode.cpp
    src/World/Object/Station/CraftingStation.cpp
    src/World/Object/Manager/ObjectManager.cpp
    src/Action/Action.cpp
    src/Action/ActionManager.cpp
    src/Movement/MovementRequest.cpp
    src/Movement/MovementDestinationRequest.cpp
    src/Pathfinding/Pathfinder.cpp
    src/Player/Player.cpp
    src/NPC/NPC.cpp
    src/Inventory/Inventory.cpp
    src/Inventory/InventorySlot.cpp
    src/Skills/Skill.cpp
    src/Skills/SkillSet.cpp
    src/Equipment/Equipment.cpp
    src/Item/ItemDefinition.cpp
    src/Item/ItemDatabase.cpp
    src/World/Object/Resource/ResourceDefinition.cpp
    src/World/Object/Resource/ResourceDatabase.cpp
    src/Recipe/RecipeDefinition.cpp
    src/Recipe/RecipeDatabase.cpp
    src/Recipe/RecipeSystem.cpp
)

add_library(GameEngineCore
    ${GAMEENGINE_CORE_SOURCES}
)

target_include_directories(GameEngineCore PUBLIC src)

add_executable(GameEngine
    src/main.cpp
    src/Core/Engine.cpp
    src/Time/Clock.cpp
    src/Input/InputManager.cpp
    src/Graphics/Graphics.cpp
)

target_link_libraries(GameEngine PRIVATE GameEngineCore SDL3::SDL3)

include(CTest)

if(BUILD_TESTING)
    add_executable(ActionFrameworkTests
        tests/ActionFrameworkTests.cpp
    )

    target_link_libraries(ActionFrameworkTests PRIVATE GameEngineCore)

    add_test(
        NAME ActionFrameworkTests
        COMMAND ActionFrameworkTests
    )

    add_executable(GameplayActionIntegrationTests
        tests/GameplayActionIntegrationTests.cpp
    )

    target_link_libraries(GameplayActionIntegrationTests PRIVATE GameEngineCore)

    add_test(
        NAME GameplayActionIntegrationTests
        COMMAND GameplayActionIntegrationTests
    )

    add_executable(EquipmentStatsTests
        tests/EquipmentStatsTests.cpp
    )

    target_link_libraries(EquipmentStatsTests PRIVATE GameEngineCore)

    add_test(
        NAME EquipmentStatsTests
        COMMAND EquipmentStatsTests
    )
endif()
