#include "TestSupport.h"

#include "../src/Core/Engine.h"
#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Player/Player.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

struct EngineTestAccess
{
    static bool InitializeWorldForRun(Engine &engine)
    {
        return engine.InitializeWorldForRun();
    }
    static bool FinalizeWorldAfterRun(Engine &engine)
    {
        return engine.FinalizeWorldAfterRun();
    }
    static World &GetWorld(Engine &engine) { return engine.world; }
    static int GetPlayerID(const Engine &engine) { return engine.playerID; }
    static WorldPlayerPersistenceLifecycle *GetLifecycle(Engine &engine)
    {
        return engine.playerPersistenceLifecycle.get();
    }
};

namespace
{
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            path = std::filesystem::temp_directory_path() /
                   ("gameengine_engine_player_lifecycle_" +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(path);
        }
        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
        std::filesystem::path path;
    };

    std::string ReadBytes(const std::filesystem::path &path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream),
                           std::istreambuf_iterator<char>());
    }

    PlayerSaveData MakeSave(int x, int y)
    {
        Player player(77, PlayerInitializationMode::DEVELOPMENT_DEFAULTS);
        player.GetPosition().SetPosition(x, y);
        return PlayerSaveState::Capture(player);
    }
}

int main()
{
    TestContext test;
    TemporaryDirectory fixture;

    {
        Engine engine;
        test.Expect(EngineTestAccess::GetLifecycle(engine) == nullptr,
                    "Default Engine constructs no persistence lifecycle");
        test.Expect(EngineTestAccess::InitializeWorldForRun(engine),
                    "Default Engine World initialization succeeds");
        World &world = EngineTestAccess::GetWorld(engine);
        test.ExpectEqual(EngineTestAccess::GetPlayerID(engine), 4,
                         "Default Engine creates development Player ID 4");
        test.Expect(dynamic_cast<Player *>(world.GetEntityByID(4)) != nullptr &&
                        world.GetEntityByID(5) == nullptr,
                    "Default Engine creates exactly one Player");
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine),
                    "Default Engine finalization is a successful no-op");
    }

    {
        const auto path = fixture.path / "missing" / "player.save";
        EngineConfiguration configuration;
        configuration.developmentPlayerSavePath = path;
        Engine engine(configuration);
        test.Expect(EngineTestAccess::InitializeWorldForRun(engine),
                    "Persistent Engine initializes a missing save");
        World &world = EngineTestAccess::GetWorld(engine);
        test.ExpectEqual(EngineTestAccess::GetPlayerID(engine), 4,
                         "Persistent missing-file startup tracks ID 4");
        test.Expect(dynamic_cast<Player *>(world.GetEntityByID(4)) != nullptr &&
                        world.GetEntityByID(5) == nullptr,
                    "Persistent startup bypasses duplicate default creation");
        test.Expect(!std::filesystem::exists(path),
                    "Engine startup does not immediately save a new Player");
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine) &&
                        std::filesystem::exists(path),
                    "Engine finalization performs controlled shutdown save");
        const std::string firstSave = ReadBytes(path);
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine) &&
                        ReadBytes(path) == firstSave,
                    "Repeated Engine finalization does not save twice");
    }

    {
        const auto path = fixture.path / "existing.save";
        test.Expect(PlayerSaveFileStore::Save(path, MakeSave(2, 3)).IsSuccess(),
                    "Existing-file Engine fixture is committed");
        const std::string original = ReadBytes(path);
        EngineConfiguration configuration;
        configuration.developmentPlayerSavePath = path;
        Engine engine(configuration);
        test.Expect(EngineTestAccess::InitializeWorldForRun(engine),
                    "Persistent Engine loads an existing save");
        World &world = EngineTestAccess::GetWorld(engine);
        auto *player = dynamic_cast<Player *>(world.GetEntityByID(4));
        test.Expect(player != nullptr && player->GetPosition().GetX() == 2 &&
                        player->GetPosition().GetY() == 3 && world.GetEntityByID(5) == nullptr,
                    "Existing-file Engine startup restores exactly one Player");
        test.Expect(ReadBytes(path) == original, "Engine startup does not rewrite existing save");
        player->GetPosition().SetPosition(4, 4);
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine) &&
                        ReadBytes(path).find("position_x=4\nposition_y=4") != std::string::npos,
                    "Engine finalization saves current authoritative state");
    }

    {
        const auto path = fixture.path / "corrupt.save";
        std::ofstream(path, std::ios::binary) << "corrupt";
        const std::string original = ReadBytes(path);
        EngineConfiguration configuration;
        configuration.developmentPlayerSavePath = path;
        Engine engine(configuration);
        test.Expect(!EngineTestAccess::InitializeWorldForRun(engine),
                    "Engine surfaces persistent startup failure");
        World &world = EngineTestAccess::GetWorld(engine);
        test.Expect(world.GetEntityByID(4) == nullptr && world.GetCurrentTick() == 0,
                    "Failed Engine startup creates no Player and performs no tick");
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine) && ReadBytes(path) == original,
                    "Finalization after failed startup performs no save");
    }

    {
        EngineConfiguration configuration;
        configuration.developmentPlayerSavePath = std::filesystem::path{};
        Engine engine(configuration);
        test.Expect(!EngineTestAccess::InitializeWorldForRun(engine),
                    "Engine rejects an explicitly empty save path");
        World &world = EngineTestAccess::GetWorld(engine);
        auto *lifecycle = EngineTestAccess::GetLifecycle(engine);
        test.Expect(lifecycle != nullptr &&
                        lifecycle->GetState() ==
                            WorldPlayerPersistenceLifecycleState::FAILED &&
                        !lifecycle->GetTrackedPlayerEntityID().has_value() &&
                        world.GetEntityByID(4) == nullptr,
                    "Empty-path Engine startup performs no World player creation");
    }

    {
        const auto parent = fixture.path / "engine_blocked_parent";
        const auto path = parent / "player.save";
        EngineConfiguration configuration;
        configuration.developmentPlayerSavePath = path;
        Engine engine(configuration);
        test.Expect(EngineTestAccess::InitializeWorldForRun(engine),
                    "Shutdown-failure Engine fixture initializes");
        std::ofstream(parent, std::ios::binary) << "regular file";
        test.Expect(!EngineTestAccess::FinalizeWorldAfterRun(engine),
                    "Engine finalization surfaces shutdown failure");
        test.Expect(EngineTestAccess::GetLifecycle(engine)->GetState() ==
                        WorldPlayerPersistenceLifecycleState::ACTIVE,
                    "Failed Engine shutdown remains retryable");
        std::filesystem::remove(parent);
        test.Expect(EngineTestAccess::FinalizeWorldAfterRun(engine),
                    "Engine shutdown succeeds after correcting path failure");
    }

    return test.Finish();
}
