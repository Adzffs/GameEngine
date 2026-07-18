cmake_minimum_required(VERSION 3.20)

project(GameEngine)
find_package(SDL3 REQUIRED CONFIG)

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
    src/Combat/CombatFormulas.cpp
    src/Combat/MeleeAttackResolver.cpp
    src/Combat/CombatService.cpp
    src/Core/SeededRandom.cpp
    src/Item/ItemDefinition.cpp
    src/Item/ItemDatabase.cpp
    src/World/Object/Resource/ResourceDefinition.cpp
    src/World/Object/Resource/ResourceDatabase.cpp
    src/Recipe/RecipeDefinition.cpp
    src/Recipe/RecipeDatabase.cpp
    src/Recipe/RecipeSystem.cpp
    src/Requirement/RequirementEvaluator.cpp
    src/Reward/RewardTable.cpp
    src/Reward/RewardTableRoller.cpp
    src/Reward/DevelopmentRewardTables.cpp
    src/Reward/RewardTableRegistry.cpp
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

    add_executable(PlayerHealthTests
        tests/PlayerHealthTests.cpp
    )

    target_link_libraries(PlayerHealthTests PRIVATE GameEngineCore)

    add_test(
        NAME PlayerHealthTests
        COMMAND PlayerHealthTests
    )

    add_executable(CombatFormulaTests
        tests/CombatFormulaTests.cpp
    )

    target_link_libraries(CombatFormulaTests PRIVATE GameEngineCore)

    add_test(
        NAME CombatFormulaTests
        COMMAND CombatFormulaTests
    )

    add_executable(MeleeAttackResolverTests
        tests/MeleeAttackResolverTests.cpp
    )

    target_link_libraries(MeleeAttackResolverTests PRIVATE GameEngineCore)

    add_test(
        NAME MeleeAttackResolverTests
        COMMAND MeleeAttackResolverTests
    )

    add_executable(CombatServiceTests
        tests/CombatServiceTests.cpp
    )

    target_link_libraries(CombatServiceTests PRIVATE GameEngineCore)

    add_test(
        NAME CombatServiceTests
        COMMAND CombatServiceTests
    )

    add_executable(MeleeCombatActionTests
        tests/MeleeCombatActionTests.cpp
    )

    target_link_libraries(MeleeCombatActionTests PRIVATE GameEngineCore)

    add_test(
        NAME MeleeCombatActionTests
        COMMAND MeleeCombatActionTests
    )

    add_executable(MonsterGraphicsTests
        tests/MonsterGraphicsTests.cpp
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(MonsterGraphicsTests PRIVATE GameEngineCore SDL3::SDL3)

    add_test(
        NAME MonsterGraphicsTests
        COMMAND MonsterGraphicsTests
    )

    add_executable(MeleeClickIntegrationTests
        tests/MeleeClickIntegrationTests.cpp
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(MeleeClickIntegrationTests PRIVATE GameEngineCore SDL3::SDL3)

    add_test(
        NAME MeleeClickIntegrationTests
        COMMAND MeleeClickIntegrationTests
    )

    add_executable(PendingMeleeInteractionTests
        tests/PendingMeleeInteractionTests.cpp
    )

    target_link_libraries(PendingMeleeInteractionTests PRIVATE GameEngineCore)

    add_test(
        NAME PendingMeleeInteractionTests
        COMMAND PendingMeleeInteractionTests
    )

    add_executable(MeleeEngagementTests
        tests/MeleeEngagementTests.cpp
    )

    target_link_libraries(MeleeEngagementTests PRIVATE GameEngineCore)

    add_test(
        NAME MeleeEngagementTests
        COMMAND MeleeEngagementTests
    )

    add_executable(CombatantMonsterTests
        tests/CombatantMonsterTests.cpp
    )

    target_link_libraries(CombatantMonsterTests PRIVATE GameEngineCore)

    add_test(
        NAME CombatantMonsterTests
        COMMAND CombatantMonsterTests
    )

    add_executable(CombatFeedbackTests
        tests/CombatFeedbackTests.cpp
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(CombatFeedbackTests PRIVATE GameEngineCore SDL3::SDL3)

    add_test(
        NAME CombatFeedbackTests
        COMMAND CombatFeedbackTests
    )

    add_executable(MonsterRetaliationTests
        tests/MonsterRetaliationTests.cpp
        src/Graphics/Graphics.cpp
    )

    target_link_libraries(MonsterRetaliationTests PRIVATE GameEngineCore SDL3::SDL3)

    add_test(
        NAME MonsterRetaliationTests
        COMMAND MonsterRetaliationTests
    )

    add_executable(EntityDeathEventTests
        tests/EntityDeathEventTests.cpp
    )

    target_link_libraries(EntityDeathEventTests PRIVATE GameEngineCore)

    add_test(
        NAME EntityDeathEventTests
        COMMAND EntityDeathEventTests
    )

    add_executable(RewardTableTests
        tests/RewardTableTests.cpp
    )

    target_link_libraries(RewardTableTests PRIVATE GameEngineCore)

    add_test(
        NAME RewardTableTests
        COMMAND RewardTableTests
    )

    add_executable(InventoryTransactionTests
        tests/InventoryTransactionTests.cpp
    )

    target_link_libraries(InventoryTransactionTests PRIVATE GameEngineCore)

    add_test(
        NAME InventoryTransactionTests
        COMMAND InventoryTransactionTests
    )

    add_executable(MonsterLootIntegrationTests
        tests/MonsterLootIntegrationTests.cpp
    )

    target_link_libraries(MonsterLootIntegrationTests PRIVATE GameEngineCore)

    add_test(
        NAME MonsterLootIntegrationTests
        COMMAND MonsterLootIntegrationTests
    )

    add_executable(MonsterRespawnTests
        tests/MonsterRespawnTests.cpp
    )

    target_link_libraries(MonsterRespawnTests PRIVATE GameEngineCore)

    add_test(
        NAME MonsterRespawnTests
        COMMAND MonsterRespawnTests
    )

    add_executable(EquipmentUnequipTests
        tests/EquipmentUnequipTests.cpp
    )

    target_link_libraries(EquipmentUnequipTests PRIVATE GameEngineCore)

    add_test(
        NAME EquipmentUnequipTests
        COMMAND EquipmentUnequipTests
    )

    add_executable(RequirementSystemTests
        tests/RequirementSystemTests.cpp
    )

    target_link_libraries(RequirementSystemTests PRIVATE GameEngineCore)

    add_test(
        NAME RequirementSystemTests
        COMMAND RequirementSystemTests
    )
endif()
