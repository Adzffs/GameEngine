cmake_minimum_required(VERSION 3.20)

project(GameEngine)

option(
    GAMEENGINE_BUILD_CLIENT
    "Build the SDL client executable"
    ON
)

option(
    GAMEENGINE_BUILD_CONTENT_TOOLS
    "Build the starter-world export and validation tools"
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
    src/Dialogue/DialogueDefinitionDatabase.cpp
    src/Dialogue/DialogueSystem.cpp
    src/Dialogue/DialoguePresentationState.cpp
    src/Shop/ShopPresentationState.cpp
    src/Shop/ShopDefinitionDatabase.cpp
    src/Shop/ShopSystem.cpp
    src/AI/MonsterAISystem.cpp
    src/Command/ServerCommandQueue.cpp
    src/Scheduling/TickScheduler.cpp
    src/Stats/StatBlock.cpp
    src/Stats/HealthPool.cpp
    src/World/World.cpp
    src/World/Position.cpp
    src/Entity/Entity.cpp
    src/Entity/Monster/Monster.cpp
    src/Entity/Manager/EntityManager.cpp
    src/Movement/Movement.cpp
    src/Movement/MovementSystem.cpp
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
    src/Persistence/PlayerSaveValidation.cpp
    src/Persistence/PlayerSaveState.cpp
    src/Persistence/PersistenceTokenCodec.cpp
    src/Persistence/PlayerSaveTextCodec.cpp
    src/Persistence/PlayerSaveFileStore.cpp
    src/Persistence/WorldPlayerPersistenceLifecycle.cpp
    src/NPC/NPC.cpp
    src/NPC/NpcDefinitionDatabase.cpp
    src/NPC/NpcSpawnDatabase.cpp
    src/NPC/NpcSpawnManager.cpp
    src/Inventory/Inventory.cpp
    src/Inventory/InventorySlot.cpp
    src/Inventory/InventoryStackOperations.cpp
    src/Skills/Skill.cpp
    src/Skills/SkillSet.cpp
    src/Equipment/Equipment.cpp
    src/StatusEffect/StatusEffectValidation.cpp
    src/StatusEffect/StatusEffectManager.cpp
    src/Combat/CombatFormulas.cpp
    src/Combat/MeleeAttackResolver.cpp
    src/Combat/CombatService.cpp
    src/Combat/MeleeEngagementSystem.cpp
    src/Gathering/GatheringSystem.cpp
    src/Quest/QuestDefinitionDatabase.cpp
    src/Quest/QuestJournal.cpp
    src/Quest/QuestSystem.cpp
    src/Interaction/InteractionSystem.cpp
    src/Time/Clock.cpp
    src/Core/SeededRandom.cpp
    src/Time/TickPerformance.cpp
    src/Item/ItemDefinition.cpp
    src/Item/ItemDatabase.cpp
    src/Item/ItemTagDatabase.cpp
    src/World/Object/Resource/ResourceDefinition.cpp
    src/World/Object/Resource/ResourceDatabase.cpp
    src/Recipe/RecipeDefinition.cpp
    src/Recipe/RecipeDatabase.cpp
    src/Recipe/RecipeBook.cpp
    src/Recipe/CraftingRecipeMatcher.cpp
    src/Recipe/CraftingSessionSystem.cpp
    src/Recipe/CraftingInteractionModel.cpp
    src/Recipe/RecipeActionSystem.cpp
    src/Recipe/RecipeSystem.cpp
    src/Requirement/RequirementEvaluator.cpp
    src/Content/ContentValidationReport.cpp
    src/Content/ContentValidator.cpp
    src/Core/ContentStartupValidation.cpp
    src/Core/GameLaunchOptions.cpp
    src/Content/Export/StarterWorldSnapshot.cpp
    src/Content/Export/StarterWorldSnapshotInput.cpp
    src/Content/Import/StarterWorldRuntimeInput.cpp
    src/Content/Edit/StarterWorldEdit.cpp
    src/Content/Edit/StarterWorldDraftFile.cpp
    src/Content/Edit/StarterWorldHistory.cpp
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

# Compatibility target: runtime input is SDL-free core functionality, so it is
# available even when the standalone content tools are omitted.
add_library(EmbervaleWorldSnapshot INTERFACE)
target_link_libraries(EmbervaleWorldSnapshot INTERFACE GameEngineCore)

if(GAMEENGINE_BUILD_CONTENT_TOOLS)

    add_executable(EmbervaleWorldExport
        src/Tools/WorldExportMain.cpp
    )
    target_link_libraries(EmbervaleWorldExport PRIVATE EmbervaleWorldSnapshot)

    add_executable(EmbervaleWorldValidate src/Tools/WorldValidateMain.cpp)
    target_link_libraries(EmbervaleWorldValidate PRIVATE GameEngineCore)

    add_executable(EmbervaleWorldMove
        src/Tools/WorldMoveMain.cpp
        src/Tools/WorldEditSession.cpp
    )
    target_link_libraries(EmbervaleWorldMove PRIVATE GameEngineCore)
endif()

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
    if(GAMEENGINE_BUILD_CLIENT)
        add_test(
            NAME GameEngineStartupCliTests
            COMMAND ${CMAKE_COMMAND}
                "-DCLIENT=$<TARGET_FILE:GameEngine>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/game-startup-cli-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/GameEngineStartupCliTests.cmake"
        )
        set_tests_properties(GameEngineStartupCliTests PROPERTIES LABELS "client;content")
    endif()
    add_gameengine_headless_test(GameLaunchOptionsTests tests/GameLaunchOptionsTests.cpp)
    add_gameengine_headless_test(StarterWorldRuntimeInputTests tests/StarterWorldRuntimeInputTests.cpp)
    add_gameengine_headless_test(StarterWorldEditTests tests/StarterWorldEditTests.cpp)
    add_gameengine_headless_test(StarterWorldDraftFileTests tests/StarterWorldDraftFileTests.cpp)
    add_gameengine_headless_test(StarterWorldHistoryTests tests/StarterWorldHistoryTests.cpp)
    add_gameengine_headless_test(StarterWorldPlacementTests tests/StarterWorldPlacementTests.cpp)
    add_gameengine_headless_test(StarterWorldRuntimeTests tests/StarterWorldRuntimeTests.cpp)
    if(GAMEENGINE_BUILD_CONTENT_TOOLS)
        add_gameengine_headless_test(StarterWorldSnapshotTests tests/StarterWorldSnapshotTests.cpp)
        target_link_libraries(StarterWorldSnapshotTests PRIVATE EmbervaleWorldSnapshot)

        add_gameengine_headless_test(StarterWorldSnapshotInputTests tests/StarterWorldSnapshotInputTests.cpp)
        target_link_libraries(StarterWorldSnapshotInputTests PRIVATE EmbervaleWorldSnapshot)

        add_test(
            NAME StarterWorldExportCliTests
            COMMAND ${CMAKE_COMMAND}
                "-DEXPORTER=$<TARGET_FILE:EmbervaleWorldExport>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/starter-world-export-cli-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldExportCliTests.cmake"
        )
        set_tests_properties(StarterWorldExportCliTests PROPERTIES LABELS "headless;content")

        add_test(
            NAME StarterWorldValidateCliTests
            COMMAND ${CMAKE_COMMAND}
                "-DVALIDATOR=$<TARGET_FILE:EmbervaleWorldValidate>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/starter-world-validate-cli-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldValidateCliTests.cmake"
        )
        set_tests_properties(StarterWorldValidateCliTests PROPERTIES LABELS "headless;content")

        add_test(
            NAME StarterWorldMoveCliTests
            COMMAND ${CMAKE_COMMAND}
                "-DMOVER=$<TARGET_FILE:EmbervaleWorldMove>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/starter-world-move-cli-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldMoveCliTests.cmake"
        )
        set_tests_properties(StarterWorldMoveCliTests PROPERTIES LABELS "headless;content")
        add_test(
            NAME StarterWorldPaintCliTests
            COMMAND ${CMAKE_COMMAND}
                "-DMOVER=$<TARGET_FILE:EmbervaleWorldMove>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/starter-world-paint-cli-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldPaintCliTests.cmake"
        )
        set_tests_properties(StarterWorldPaintCliTests PROPERTIES LABELS "headless;content")
        add_test(
            NAME StarterWorldInteractiveSessionTests
            COMMAND ${CMAKE_COMMAND}
                "-DMOVER=$<TARGET_FILE:EmbervaleWorldMove>"
                "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/starter-world-session-test"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldInteractiveSessionTests.cmake"
        )
        set_tests_properties(StarterWorldInteractiveSessionTests PROPERTIES LABELS "headless;content")

        # Optional local test crosses the real C++ editor and Tk bridge.
        # Python is optional; absence means no integration test registered.
        find_package(Python3 COMPONENTS Interpreter QUIET)
        if(Python3_Interpreter_FOUND)
            add_test(
                NAME StarterWorldVisualBridgeIntegration
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StarterWorldVisualBridgeIntegration.py"
                    --tool "$<TARGET_FILE:EmbervaleWorldMove>"
                    --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/content/development-starter.snapshot.json"
            )
            set_tests_properties(StarterWorldVisualBridgeIntegration
                PROPERTIES LABELS "headless;content;studio")
        endif()
    endif()

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
    add_gameengine_headless_test(MeleeEngagementSystemTests tests/MeleeEngagementSystemTests.cpp)
    add_gameengine_headless_test(InteractionSystemTests tests/InteractionSystemTests.cpp)
    add_gameengine_headless_test(CombatantMonsterTests tests/CombatantMonsterTests.cpp)
    add_gameengine_headless_test(EntityDeathEventTests tests/EntityDeathEventTests.cpp)
    add_gameengine_headless_test(RewardTableTests tests/RewardTableTests.cpp)
    add_gameengine_headless_test(InventoryTransactionTests tests/InventoryTransactionTests.cpp)
    add_gameengine_headless_test(InventoryStackOperationsTests tests/InventoryStackOperationsTests.cpp)
    add_gameengine_headless_test(PlayerSaveStateTests tests/PlayerSaveStateTests.cpp)
    add_gameengine_headless_test(PlayerSaveTextCodecTests tests/PlayerSaveTextCodecTests.cpp)
    add_gameengine_headless_test(PlayerSaveFileStoreTests tests/PlayerSaveFileStoreTests.cpp)
    add_gameengine_headless_test(WorldPlayerPersistenceTests tests/WorldPlayerPersistenceTests.cpp)
    add_gameengine_headless_test(WorldPlayerPersistenceLifecycleTests tests/WorldPlayerPersistenceLifecycleTests.cpp)

    if(GAMEENGINE_BUILD_CLIENT OR GAMEENGINE_BUILD_GRAPHICS_TESTS)
        add_executable(EnginePlayerPersistenceLifecycleTests
            tests/EnginePlayerPersistenceLifecycleTests.cpp
            src/Core/Engine.cpp
            src/Input/InputManager.cpp
            src/Graphics/Graphics.cpp
        )
        target_link_libraries(EnginePlayerPersistenceLifecycleTests PRIVATE GameEngineCore SDL3::SDL3)
        add_test(NAME EnginePlayerPersistenceLifecycleTests COMMAND EnginePlayerPersistenceLifecycleTests)
        set_tests_properties(EnginePlayerPersistenceLifecycleTests PROPERTIES LABELS "client")
    endif()
    add_gameengine_headless_test(MonsterLootIntegrationTests tests/MonsterLootIntegrationTests.cpp)
    add_gameengine_headless_test(MonsterRespawnTests tests/MonsterRespawnTests.cpp)
    add_gameengine_headless_test(EquipmentUnequipTests tests/EquipmentUnequipTests.cpp)
    add_gameengine_headless_test(RequirementSystemTests tests/RequirementSystemTests.cpp)
    add_gameengine_headless_test(ActionLifecycleEventTests tests/ActionLifecycleEventTests.cpp)
    add_gameengine_headless_test(ContentValidationTests tests/ContentValidationTests.cpp)
    add_gameengine_headless_test(StatusEffectTests tests/StatusEffectTests.cpp)
    add_gameengine_headless_test(FoodHealingTests tests/FoodHealingTests.cpp)
    add_gameengine_headless_test(AggressiveMonsterTests tests/AggressiveMonsterTests.cpp)
    add_gameengine_headless_test(MonsterAISystemTests tests/MonsterAISystemTests.cpp)
    add_gameengine_headless_test(WorldTickAuthorityTests tests/WorldTickAuthorityTests.cpp)
    add_gameengine_headless_test(ClockPacingTests tests/ClockPacingTests.cpp)
    add_gameengine_headless_test(GatheringRandomTests tests/GatheringRandomTests.cpp)
    add_gameengine_headless_test(GatheringSystemTests tests/GatheringSystemTests.cpp)
    add_gameengine_headless_test(QuestSystemTests tests/QuestSystemTests.cpp)
    add_gameengine_headless_test(QuestDefinitionTests tests/QuestDefinitionTests.cpp)
    add_gameengine_headless_test(QuestWorldIntegrationTests tests/QuestWorldIntegrationTests.cpp)
    add_gameengine_headless_test(QuestDialogueIntegrationTests tests/QuestDialogueIntegrationTests.cpp)
    add_gameengine_headless_test(QuestPersistenceTests tests/QuestPersistenceTests.cpp)
    add_gameengine_headless_test(QuestPresentationStateTests tests/QuestPresentationStateTests.cpp)
    add_gameengine_headless_test(ProgressionPresentationStateTests tests/ProgressionPresentationStateTests.cpp)
    add_gameengine_headless_test(StarterRegionTests tests/StarterRegionTests.cpp)
    add_gameengine_headless_test(MiningBasicsQuestTests tests/MiningBasicsQuestTests.cpp)
    add_gameengine_headless_test(RecipeActionSystemTests tests/RecipeActionSystemTests.cpp)
    add_gameengine_headless_test(RecipeBookTests tests/RecipeBookTests.cpp)
    add_gameengine_headless_test(RecipeDiscoveryIntegrationTests tests/RecipeDiscoveryIntegrationTests.cpp)
    add_gameengine_headless_test(CraftingRecipeTests tests/CraftingRecipeTests.cpp)
    add_gameengine_headless_test(CraftingTableWorldTests tests/CraftingTableWorldTests.cpp)
    add_gameengine_headless_test(CraftingSessionTests tests/CraftingSessionTests.cpp)
    add_gameengine_headless_test(MovementPathSafetyTests tests/MovementPathSafetyTests.cpp)
    add_gameengine_headless_test(MovementSystemTests tests/MovementSystemTests.cpp)
    add_gameengine_headless_test(EntityManagerLookupTests tests/EntityManagerLookupTests.cpp)
    add_gameengine_headless_test(ServerCommandQueueTests tests/ServerCommandQueueTests.cpp)
    add_gameengine_headless_test(TickSchedulerTests tests/TickSchedulerTests.cpp)
    add_gameengine_headless_test(NpcDefinitionTests tests/NpcDefinitionTests.cpp)
    add_gameengine_headless_test(NpcSpawnManagerTests tests/NpcSpawnManagerTests.cpp)
    add_gameengine_headless_test(MonsterWanderingTests tests/MonsterWanderingTests.cpp)
    add_gameengine_headless_test(FriendlyNpcTests tests/FriendlyNpcTests.cpp)
    add_gameengine_headless_test(NpcInteractionTests tests/NpcInteractionTests.cpp)
    add_gameengine_headless_test(DialogueDefinitionTests tests/DialogueDefinitionTests.cpp)
    add_gameengine_headless_test(DialogueSystemTests tests/DialogueSystemTests.cpp)
    add_gameengine_headless_test(DialogueChoiceTests tests/DialogueChoiceTests.cpp)
    add_gameengine_headless_test(DialoguePresentationStateTests tests/DialoguePresentationStateTests.cpp)
    add_gameengine_headless_test(EngineWorldClickDecisionTests tests/EngineWorldClickDecisionTests.cpp)
    add_gameengine_headless_test(ShopPresentationStateTests tests/ShopPresentationStateTests.cpp)
    add_gameengine_headless_test(ShopDefinitionTests tests/ShopDefinitionTests.cpp)
    add_gameengine_headless_test(ShopSystemTests tests/ShopSystemTests.cpp)
    add_gameengine_headless_test(ShopInteractionTests tests/ShopInteractionTests.cpp)

    if(GAMEENGINE_BUILD_GRAPHICS_TESTS)
        add_gameengine_graphics_test(MonsterGraphicsTests tests/MonsterGraphicsTests.cpp)
        add_gameengine_graphics_test(MeleeClickIntegrationTests tests/MeleeClickIntegrationTests.cpp)
        add_gameengine_graphics_test(CombatFeedbackTests tests/CombatFeedbackTests.cpp)
        add_gameengine_graphics_test(CameraPresentationTests tests/CameraPresentationTests.cpp)
        add_gameengine_graphics_test(MonsterRetaliationTests tests/MonsterRetaliationTests.cpp)
        add_gameengine_graphics_test(QuestGraphicsTests tests/QuestGraphicsTests.cpp)
        add_gameengine_graphics_test(CraftingTableGraphicsTests tests/CraftingTableGraphicsTests.cpp)

        add_executable(ClientDialogueIntegrationTests
            tests/ClientDialogueIntegrationTests.cpp
            src/Core/Engine.cpp
            src/Input/InputManager.cpp
            src/Graphics/Graphics.cpp
        )
        target_link_libraries(ClientDialogueIntegrationTests PRIVATE GameEngineCore SDL3::SDL3)
        add_test(NAME ClientDialogueIntegrationTests COMMAND ClientDialogueIntegrationTests)
        set_tests_properties(ClientDialogueIntegrationTests PROPERTIES LABELS "client")
        add_executable(ClientShopIntegrationTests
            tests/ClientShopIntegrationTests.cpp
            src/Core/Engine.cpp
            src/Input/InputManager.cpp
            src/Graphics/Graphics.cpp)
        target_link_libraries(ClientShopIntegrationTests PRIVATE GameEngineCore SDL3::SDL3)
        add_test(NAME ClientShopIntegrationTests COMMAND ClientShopIntegrationTests)
        set_tests_properties(ClientShopIntegrationTests PROPERTIES LABELS "client")

        add_executable(ClientCraftingIntegrationTests
            tests/ClientCraftingIntegrationTests.cpp
            src/Core/Engine.cpp
            src/Input/InputManager.cpp
            src/Graphics/Graphics.cpp)
        target_link_libraries(ClientCraftingIntegrationTests PRIVATE GameEngineCore SDL3::SDL3)
        add_test(NAME ClientCraftingIntegrationTests COMMAND ClientCraftingIntegrationTests)
        set_tests_properties(ClientCraftingIntegrationTests PROPERTIES LABELS "client")
    endif()
endif()
