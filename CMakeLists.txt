cmake_minimum_required(VERSION 3.20)

project(GameEngine)

option(
    GAMEENGINE_BUILD_CLIENT
    "Build the SDL client executable"
    ON
)

set(GAMEENGINE_DEFAULT_BUILD_GRAPHICS_TESTS ${GAMEENGINE_BUILD_CLIENT})
option(
    GAMEENGINE_BUILD_GRAPHICS_TESTS
    "Build tests that require SDL graphics/input"
    ${GAMEENGINE_DEFAULT_BUILD_GRAPHICS_TESTS}
)

if(
    GAMEENGINE_BUILD_CLIENT
    OR GAMEENGINE_BUILD_GRAPHICS_TESTS
)
    find_package(SDL3 REQUIRED CONFIG)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAMEENGINE_CORE_SOURCES
    src/Stats/StatBlock.cpp
    src/Stats/HealthPool.cpp
    src/World/World.cpp
    src/World/Position.cpp
    src/Entity/Entity.cpp
    src/Entity/Monster/Monster.cpp
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
    src/StatusEffect/StatusEffectValidation.cpp
    src/StatusEffect/StatusEffectManager.cpp
    src/Combat/CombatFormulas.cpp
    src/Combat/MeleeAttackResolver.cpp
    src/Combat/CombatService.cpp
    src/Time/Clock.cpp
    src/Core/SeededRandom.cpp
    src/Time/TickPerformance.cpp
    src/Item/ItemDefinition.cpp
    src/Item/ItemDatabase.cpp
    src/World/Object/Resource/ResourceDefinition.cpp
    src/World/Object/Resource/ResourceDatabase.cpp
    src/Recipe/RecipeDefinition.cpp
    src/Recipe/RecipeDatabase.cpp
    src/Recipe/RecipeSystem.cpp
    src/Requirement/RequirementEvaluator.cpp
    src/Content/ContentValidationReport.cpp
    src/Content/ContentValidator.cpp
    src/Core/ContentStartupValidation.cpp
    src/Reward/RewardTable.cpp
    src/Reward/RewardTableRoller.cpp
    src/Reward/DevelopmentRewardTables.cpp
    src/Reward/RewardTableRegistry.cpp
    src/World/Development/DevelopmentWorldContent.cpp
)

add_library(GameEngineCore
    ${GAMEENGINE_CORE_SOURCES}
)

target_include_directories(GameEngineCore PUBLIC src)

if(GAMEENGINE_BUILD_CLIENT)
    add_executable(GameEngine
        src/main.cpp
        src/Core/Engine.cpp
        src/Input/InputManager.cpp
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(GameEngine PRIVATE GameEngineCore SDL3::SDL3)
endif()

include(CTest)

function(add_gameengine_headless_test target source)
    add_executable(${target}
        ${source}
    )

    target_link_libraries(${target} PRIVATE GameEngineCore)

    add_test(
        NAME ${target}
        COMMAND ${target}
    )

    set_tests_properties(
        ${target}
        PROPERTIES LABELS "headless"
    )
endfunction()

function(add_gameengine_graphics_test target source)
    add_executable(${target}
        ${source}
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(${target} PRIVATE GameEngineCore SDL3::SDL3)

    add_test(
        NAME ${target}
        COMMAND ${target}
    )

    set_tests_properties(
        ${target}
        PROPERTIES LABELS "graphics"
    )
endfunction()

if(BUILD_TESTING)
    add_gameengine_headless_test(ActionFrameworkTests tests/ActionFrameworkTests.cpp)
    add_gameengine_headless_test(GameplayActionIntegrationTests tests/GameplayActionIntegrationTests.cpp)
    add_gameengine_headless_test(EquipmentStatsTests tests/EquipmentStatsTests.cpp)
    add_gameengine_headless_test(PlayerHealthTests tests/PlayerHealthTests.cpp)
    add_gameengine_headless_test(CombatFormulaTests tests/CombatFormulaTests.cpp)
    add_gameengine_headless_test(MeleeAttackResolverTests tests/MeleeAttackResolverTests.cpp)
    add_gameengine_headless_test(CombatServiceTests tests/CombatServiceTests.cpp)
    add_gameengine_headless_test(MeleeCombatActionTests tests/MeleeCombatActionTests.cpp)
    add_gameengine_headless_test(PendingMeleeInteractionTests tests/PendingMeleeInteractionTests.cpp)
    add_gameengine_headless_test(MeleeEngagementTests tests/MeleeEngagementTests.cpp)
    add_gameengine_headless_test(CombatantMonsterTests tests/CombatantMonsterTests.cpp)
    add_gameengine_headless_test(EntityDeathEventTests tests/EntityDeathEventTests.cpp)
    add_gameengine_headless_test(RewardTableTests tests/RewardTableTests.cpp)
    add_gameengine_headless_test(InventoryTransactionTests tests/InventoryTransactionTests.cpp)
    add_gameengine_headless_test(MonsterLootIntegrationTests tests/MonsterLootIntegrationTests.cpp)
    add_gameengine_headless_test(MonsterRespawnTests tests/MonsterRespawnTests.cpp)
    add_gameengine_headless_test(EquipmentUnequipTests tests/EquipmentUnequipTests.cpp)
    add_gameengine_headless_test(RequirementSystemTests tests/RequirementSystemTests.cpp)
    add_gameengine_headless_test(ActionLifecycleEventTests tests/ActionLifecycleEventTests.cpp)
    add_gameengine_headless_test(ContentValidationTests tests/ContentValidationTests.cpp)
    add_gameengine_headless_test(StatusEffectTests tests/StatusEffectTests.cpp)
    add_gameengine_headless_test(FoodHealingTests tests/FoodHealingTests.cpp)
    add_gameengine_headless_test(AggressiveMonsterTests tests/AggressiveMonsterTests.cpp)
    add_gameengine_headless_test(WorldTickAuthorityTests tests/WorldTickAuthorityTests.cpp)
    add_gameengine_headless_test(ClockPacingTests tests/ClockPacingTests.cpp)
    add_gameengine_headless_test(GatheringRandomTests tests/GatheringRandomTests.cpp)

    if(GAMEENGINE_BUILD_GRAPHICS_TESTS)
        add_gameengine_graphics_test(MonsterGraphicsTests tests/MonsterGraphicsTests.cpp)
        add_gameengine_graphics_test(MeleeClickIntegrationTests tests/MeleeClickIntegrationTests.cpp)
        add_gameengine_graphics_test(CombatFeedbackTests tests/CombatFeedbackTests.cpp)
        add_gameengine_graphics_test(MonsterRetaliationTests tests/MonsterRetaliationTests.cpp)
    endif()
endif()
