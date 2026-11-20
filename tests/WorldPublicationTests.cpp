#include "TestSupport.h"
#include "../src/Content/Publication/WorldPublication.h"
#include "../src/Content/Edit/StarterWorldDraftFile.h"
#include "../src/Player/Player.h"
#include "../src/World/World.h"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace
{
    struct Fixture
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("embervale-published-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Fixture() { std::filesystem::create_directory(path); }
        ~Fixture() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    };
    void Write(const std::filesystem::path &path, const std::string &bytes)
    { std::ofstream file(path, std::ios::binary); file << bytes; }
    template<class F> bool Reject(F operation)
    { try { operation(); } catch (const std::exception &) { return true; } return false; }
    std::string Replace(std::string input, const std::string &old, const std::string &value)
    {
        const auto position = input.find(old);
        if (position == std::string::npos) throw std::runtime_error("bad test substitution");
        input.replace(position, old.size(), value); return input;
    }
}

int main()
{
    using namespace WorldPublication;
    TestContext test;
    Fixture f;
    const auto source = f.path / "draft.json", library = f.path / "library";
    const auto baseline = StarterWorldExport::Capture();
    const auto worldBytes = StarterWorldExport::SerializeCanonical(baseline);
    Write(source, "\n" + worldBytes);
    const auto receipt = ValidateDraft(source);
    test.Expect(!std::filesystem::exists(library) &&
        !std::filesystem::exists(source.string() + ".embervale.writer.lock"),
        "Validation is read-only, including lock and library paths");
    test.Expect(receipt.sourceSha256 != receipt.worldSha256,
        "Raw source fingerprint remains distinct from canonical published content");
    test.Expect(ParseReceipt(SerializeReceipt(receipt)) == receipt,
        "Version-bound receipt round-trips exactly");
    test.Expect(StarterWorldDraftFile::ValidDigest(ValidatorRevision()),
        "Built validation source revision is an explicit SHA-256");

    const auto manifest = Publish(source, receipt, library, "v1");
    const auto manifestPath = library / "v1/manifest.json";
    const auto manifestBytes = StarterWorldDraftFile::Read(manifestPath);
    test.Expect(ParseManifest(manifestBytes) == manifest, "Immutable manifest binds its exact receipt");
    auto loaded = Load(manifestPath);
    test.Expect(StarterWorldExport::SerializeCanonical(loaded.snapshot) == worldBytes,
        "Fresh runtime load returns the exact validated canonical world");
    World first(loaded.snapshot);
    const int firstID = first.CreatePlayer(PlayerInitializationMode::FRESH_START);
    const auto *firstPlayer = dynamic_cast<Player *>(first.GetEntityByID(firstID));
    test.Expect(firstPlayer && firstPlayer->GetPosition().GetX() == baseline.playerSpawn.GetX(),
        "Fresh world session uses selected published spawn");

    auto second = baseline;
    second.playerSpawn.SetPosition(40, 20);
    second.terrainRows[20][41] = 'S';
    const auto secondSource = f.path / "second.json";
    Write(secondSource, StarterWorldExport::SerializeCanonical(second));
    const auto secondManifest = Publish(secondSource, ValidateDraft(secondSource), library, "v2");
    World secondWorld(Load(library / "v2/manifest.json").snapshot);
    const int secondID = secondWorld.CreatePlayer(PlayerInitializationMode::FRESH_START);
    test.Expect(secondWorld.GetEntityByID(secondID)->GetPosition().GetX() == 40 &&
        secondManifest.validation.worldSha256 != manifest.validation.worldSha256,
        "A fresh second session uses the second version's different geography/spawn");
    World rollback(Load(manifestPath).snapshot);
    const int rollbackID = rollback.CreatePlayer(PlayerInitializationMode::FRESH_START);
    test.Expect(rollback.GetEntityByID(rollbackID)->GetPosition().GetX() == baseline.playerSpawn.GetX() &&
        firstPlayer->GetPosition().GetX() == baseline.playerSpawn.GetX(),
        "Rollback selects old content without mutating an existing world or either version");
    test.Expect(Reject([&]{ Publish(source, receipt, library, "v1"); }) &&
        StarterWorldDraftFile::Read(manifestPath) == manifestBytes,
        "Published names cannot be overwritten or replaced");

    Write(source, " " + worldBytes);
    const auto untouched = f.path / "uncreated";
    test.Expect(Reject([&]{ Publish(source, receipt, untouched, "stale"); }) &&
        !std::filesystem::exists(untouched),
        "Even whitespace source changes invalidate a receipt before filesystem mutation");
    auto wrongTool = ValidateDraft(source);
    wrongTool.validatorRevision = std::string(64, '0');
    test.Expect(Reject([&]{ Publish(source, wrongTool, untouched, "old-tool"); }) &&
        !std::filesystem::exists(untouched), "Receipt from another validation revision requires revalidation");

    for (const auto name : {"", "..", "../escape", "Uppercase", "-leading", "trailing-", "a/b", "con", "com1"})
        test.Expect(!ValidRevision(name), std::string("Invalid revision name rejects: ") + name);
    test.Expect(Reject([&]{ ParseReceipt(Replace(SerializeReceipt(receipt),
        "\"receipt_version\": 1", "\"receipt_version\": true")); }), "Receipt version requires an integer");
    test.Expect(Reject([&]{ ParseManifest(Replace(manifestBytes, "\"world.json\"", "\"../world.json\"")); }),
        "Manifest cannot select a parent/path-traversal payload");
    test.Expect(Reject([&]{ ParseManifest(Replace(manifestBytes, "\"manifest_version\": 1",
        "\"manifest_version\": 2")); }), "Unsupported publication schema rejects");
    test.Expect(Reject([&]{ ParseManifest(Replace(manifestBytes, "\"manifest_version\": 1",
        "\"manifest_version\": 1, \"manifest_version\": 1")); }), "Duplicate metadata keys reject");
    test.Expect(Reject([&]{ ParseManifest(std::string(MaximumMetadataBytes + 1, ' ')); }),
        "Metadata byte limit applies before DOM parsing");

    const auto publishedWorld = library / "v1/world.json";
    Write(publishedWorld, worldBytes + "\n");
    test.Expect(Reject([&]{ Load(manifestPath); }), "Payload mutation is detected by its exact SHA-256");
    Write(publishedWorld, worldBytes);
    auto oldValidator = manifest;
    oldValidator.validation.validatorRevision = std::string(64, '0');
    Write(manifestPath, SerializeManifest(oldValidator));
    test.Expect(Load(manifestPath).manifest == oldValidator,
        "Old validation revisions remain usable only through current-profile revalidation");
    auto unsafe = baseline;
    unsafe.terrainRows[unsafe.playerSpawn.GetY()][unsafe.playerSpawn.GetX()] = 'W';
    const auto unsafeBytes = StarterWorldExport::SerializeCanonical(unsafe);
    Write(publishedWorld, unsafeBytes);
    oldValidator.validation.worldSha256 = StarterWorldDraftFile::Digest(unsafeBytes);
    Write(manifestPath, SerializeManifest(oldValidator));
    test.Expect(Reject([&]{ Load(manifestPath); }),
        "A matching edited hash and old receipt never bypass current spawn/route validation");
    Write(publishedWorld, worldBytes);
    Write(manifestPath, manifestBytes);
    test.Expect(Reject([&]{ Load(library / "v1/world.json"); }), "Runtime requires an explicit manifest");
    test.Expect(StarterWorldDraftFile::Read(source) == " " + worldBytes,
        "All publication/load failures leave the editable source intact");
    return test.Finish();
}
