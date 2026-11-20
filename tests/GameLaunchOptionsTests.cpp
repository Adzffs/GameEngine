#include "TestSupport.h"
#include "../src/Core/GameLaunchOptions.h"

#include <initializer_list>
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace
{
    GameLaunchOptions Parse(std::initializer_list<std::string_view> arguments)
    {
        return ParseGameLaunchOptions({arguments.begin(), arguments.size()});
    }

    void Reject(TestContext &test, std::initializer_list<std::string_view> arguments,
        const std::string &message)
    {
        bool rejected = false;
        try { (void)Parse(arguments); }
        catch (const std::invalid_argument &) { rejected = true; }
        test.Expect(rejected, message);
    }
}

int main()
{
    TestContext test;
    const auto normal = Parse({});
    test.Expect(!normal.worldSnapshotPath, "normal startup uses compiled world");
    test.Expect(normal.playerSavePath == std::filesystem::path{"saves/development-player.save"},
        "normal startup retains established save location");
    const auto imported = Parse({"--world", "world.json"});
    test.Expect(imported.worldSnapshotPath == std::filesystem::path{"world.json"},
        "explicit world selection is retained");
    test.Expect(!imported.playerSavePath, "edited-world playtest never implicitly opens normal save");
    const auto persisted = Parse({"--save", "playtest.save", "--world", "world.json"});
    test.Expect(persisted.playerSavePath == std::filesystem::path{"playtest.save"},
        "explicit imported-world save honored regardless of option order");
    test.Expect(!Parse({"--compiled-world", "--no-save"}).playerSavePath,
        "compiled fallback can use disposable profile");
    test.Expect(Parse({"--help"}).showHelp, "help is parsed without loading content");
    const auto version = Parse({"--world-version", "library/v1/manifest.json"});
    test.Expect(version.worldVersionManifestPath == std::filesystem::path("library/v1/manifest.json") &&
        !version.worldSnapshotPath && !version.playerSavePath,
        "Published versions select an explicit manifest and temporary progress by default");
    Reject(test, {"--world-version"}, "Missing manifest rejects");
    Reject(test, {"--world-version", "a.json", "--world", "b.json"}, "World sources conflict");
    Reject(test, {"--compiled-world", "--world-version", "a.json"}, "Compiled and published sources conflict");
    Reject(test, {"--world-version", "a.json", "--world-version", "b.json"}, "Duplicate manifests reject");
    Reject(test, {"--world"}, "missing snapshot path rejected");
    Reject(test, {"--world", ""}, "empty snapshot path rejected");
    Reject(test, {"--save", "--no-save"}, "a flag is not a filename");
    Reject(test, {"--world", "a.json", "--world", "b.json"}, "duplicate world rejected");
    Reject(test, {"--compiled-world", "--world", "a.json"}, "conflicting world sources rejected");
    Reject(test, {"--world", "a.json", "--compiled-world"}, "reverse source conflict rejected");
    Reject(test, {"--save", "a.save", "--no-save"}, "conflicting save policy rejected");
    Reject(test, {"--no-save", "--save", "a.save"}, "reverse save conflict rejected");
    Reject(test, {"--save", "a.save", "--save", "b.save"}, "duplicate save target rejected");
    Reject(test, {"--world", "a.json", "--help"}, "help cannot hide invalid startup arguments");
    Reject(test, {"--unknown"}, "unknown startup argument rejected");

    const auto directory = std::filesystem::temp_directory_path() /
        ("embervale-launch-paths-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory / "nested");
    const auto world = directory / "world.json";
    { std::ofstream file(world); file << "world-input-must-survive"; }
    const auto rejectPaths = [&](const std::filesystem::path &input,
        const std::filesystem::path &save, const std::string &message) {
        GameLaunchOptions options;
        options.worldSnapshotPath = input;
        options.playerSavePath = save;
        bool rejected = false;
        try { ValidateGameLaunchPaths(options); }
        catch (const std::invalid_argument &) { rejected = true; }
        test.Expect(rejected, message);
    };
    rejectPaths(world, world, "save cannot overwrite selected content");
    rejectPaths(world, directory / "nested/../world.json", "normalized alias is rejected");
    rejectPaths(directory / "player.save.tmp", directory / "player.save",
        "save temporary sidecar cannot alias content even before it exists");
    rejectPaths(directory / "player.save.bak", directory / "player.save",
        "save recovery sidecar cannot alias content");
    std::error_code linkError;
    const auto hardlink = directory / "linked.save";
    std::filesystem::create_hard_link(world, hardlink, linkError);
    if (!linkError) rejectPaths(world, hardlink, "hardlinked save target is rejected");
    const auto symlink = directory / "symbolic.save";
    std::filesystem::create_symlink(world, symlink, linkError);
    if (!linkError) rejectPaths(world, symlink, "symlinked save target is rejected");
    GameLaunchOptions separate;
    separate.worldSnapshotPath = world;
    separate.playerSavePath = directory / "player.save";
    ValidateGameLaunchPaths(separate);
    std::filesystem::create_directories(directory / "versions/v1");
    { std::ofstream file(directory / "versions/v1/manifest.json"); file << "protected manifest"; }
    { std::ofstream file(directory / "versions/v1/world.json"); file << "protected world"; }
    GameLaunchOptions published;
    published.worldVersionManifestPath = directory / "versions/v1/manifest.json";
    published.playerSavePath = directory / "versions/v1/player.save";
    test.Expect([&]{ try { ValidateGameLaunchPaths(published); } catch(const std::invalid_argument &) { return true; } return false; }(),
        "Player saves cannot pollute a selected immutable version");
    published.playerSavePath = directory / "versions/other/player.save";
    test.Expect([&]{ try { ValidateGameLaunchPaths(published); } catch(const std::invalid_argument &) { return true; } return false; }(),
        "Player saves must remain outside the whole selected version library");
    published.playerSavePath = directory / "persistent-character.save";
    ValidateGameLaunchPaths(published);
    test.Expect(!std::filesystem::exists(*published.playerSavePath), "Published save-path checks are read-only");
    test.Expect(!std::filesystem::exists(*separate.playerSavePath), "path validation is read-only");
    std::ifstream intact(world);
    std::string contents;
    std::getline(intact, contents);
    test.ExpectEqual(contents, std::string{"world-input-must-survive"}, "rejected aliases leave input intact");
    intact.close();
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    return test.Finish();
}
