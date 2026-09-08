#include "TestSupport.h"

#include "../src/Persistence/PlayerSaveFileStore.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/PlayerSaveTextCodec.h"
#include "../src/Player/Player.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"

#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            static std::atomic<unsigned long long> counter{0};
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() /
                ("gameengine_player_save_store_" + std::to_string(stamp) + "_" +
                 std::to_string(counter.fetch_add(1)));
            std::error_code error;
            std::filesystem::remove_all(path, error);
            std::filesystem::create_directories(path);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        const std::filesystem::path &GetPath() const { return path; }

    private:
        std::filesystem::path path;
    };

    class CurrentPathGuard
    {
    public:
        explicit CurrentPathGuard(const std::filesystem::path &path)
            : previous(std::filesystem::current_path())
        { std::filesystem::current_path(path); }
        ~CurrentPathGuard()
        {
            std::error_code error;
            std::filesystem::current_path(previous, error);
        }
    private:
        std::filesystem::path previous;
    };

    PlayerSaveData ValidSave()
    {
        Player player(1, PlayerInitializationMode::EMPTY);
        PlayerSaveData save = PlayerSaveState::Capture(player);
        save.positionX = -12;
        save.positionY = 34;
        save.currentHealth = 73;
        save.inventorySlots[0] = {ItemType::COINS, 900};
        save.inventorySlots[7] = {ItemType::BRONZE_AXE, 1};
        save.skills[0].xp = 100;
        save.skills[2].xp = 1200;
        save.equipment[3].itemType = ItemType::BRONZE_SWORD;
        save.equipment[4].itemType = ItemType::WOODEN_SHIELD;
        return save;
    }

    std::string Encode(const PlayerSaveData &save)
    {
        std::string text;
        PlayerSaveValidationReport report;
        PlayerSaveTextCodec::TryEncode(save, text, report);
        return text;
    }

    void WriteBytes(const std::filesystem::path &path, const std::string &bytes)
    {
        std::ofstream output(path, std::ios::binary | std::ios::out | std::ios::trunc);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    std::string ReadBytes(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::in);
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    bool SameSave(const PlayerSaveData &left, const PlayerSaveData &right)
    {
        if (left.version != right.version || left.positionX != right.positionX ||
            left.positionY != right.positionY || left.currentHealth != right.currentHealth ||
            left.skills.size() != right.skills.size() || left.equipment.size() != right.equipment.size()) return false;
        for (std::size_t i = 0; i < left.inventorySlots.size(); ++i)
            if (left.inventorySlots[i].itemType != right.inventorySlots[i].itemType ||
                left.inventorySlots[i].quantity != right.inventorySlots[i].quantity) return false;
        for (std::size_t i = 0; i < left.skills.size(); ++i)
            if (left.skills[i].skillType != right.skills[i].skillType || left.skills[i].xp != right.skills[i].xp) return false;
        for (std::size_t i = 0; i < left.equipment.size(); ++i)
            if (left.equipment[i].slotType != right.equipment[i].slotType ||
                left.equipment[i].itemType != right.equipment[i].itemType) return false;
        return true;
    }

    void ExpectEncodeFailurePreserves(TestContext &test, const std::filesystem::path &target,
                                      const PlayerSaveData &invalid, const std::string &name)
    {
        const std::string before = ReadBytes(target);
        const auto result = PlayerSaveFileStore::Save(target, invalid);
        test.Expect(!result.IsSuccess() && result.Contains(PlayerSaveFileIssueCode::ENCODE_FAILED), name + " rejects");
        test.ExpectEqual(ReadBytes(target), before, name + " preserves existing target");
    }
}

int main()
{
    TestContext test;
    TemporaryDirectory fixture;
    const auto root = fixture.GetPath();
    const auto target = root / "nested" / "player.save";
    const auto temporary = std::filesystem::path(target.string() + ".tmp");
    const auto backup = std::filesystem::path(target.string() + ".bak");
    const PlayerSaveData original = ValidSave();
    const std::string canonical = Encode(original);

    auto saveResult = PlayerSaveFileStore::Save(target, original);
    test.Expect(saveResult.IsSuccess() && saveResult.WasCommitted(), "valid save commits");
    test.Expect(std::filesystem::is_directory(target.parent_path()), "nested parent directories created");
    test.ExpectEqual(ReadBytes(target), canonical, "target contains exact canonical bytes");
    test.Expect(canonical.back() == '\n' && canonical.find('\r') == std::string::npos, "stored canonical bytes retain LF and final newline");
    test.Expect(!std::filesystem::exists(temporary) && !std::filesystem::exists(backup), "successful first save leaves no sidecars");
    test.Expect(canonical.find("entity") == std::string::npos, "runtime entity ID absent from file");

    PlayerSaveData replacement = original;
    replacement.positionX = 77;
    replacement.inventorySlots[0].quantity = 901;
    const std::string replacementText = Encode(replacement);
    saveResult = PlayerSaveFileStore::Save(target, replacement);
    test.Expect(saveResult.IsSuccess(), "repeated save replaces target");
    test.ExpectEqual(ReadBytes(target), replacementText, "previous contents replaced");
    test.Expect(!std::filesystem::exists(temporary) && !std::filesystem::exists(backup), "replacement cleans sidecars");
    test.Expect(PlayerSaveFileStore::Save(target, replacement).IsSuccess() && ReadBytes(target) == replacementText,
                "identical repeated save remains byte-identical");

    PlayerSaveData invalid = original; invalid.version = 1;
    ExpectEncodeFailurePreserves(test, target, invalid, "unsupported version");
    invalid = original; invalid.currentHealth = 0;
    ExpectEncodeFailurePreserves(test, target, invalid, "invalid health");
    invalid = original; invalid.inventorySlots[0] = {ItemType::NONE, 1};
    ExpectEncodeFailurePreserves(test, target, invalid, "invalid inventory");
    invalid = original; invalid.skills.pop_back();
    ExpectEncodeFailurePreserves(test, target, invalid, "invalid skills");
    invalid = original; invalid.equipment[0].itemType = ItemType::BRONZE_SWORD;
    ExpectEncodeFailurePreserves(test, target, invalid, "invalid equipment");
    const auto absentParent = root / "must_not_exist" / "player.save";
    invalid = original; invalid.currentHealth = 0;
    test.Expect(!PlayerSaveFileStore::Save(absentParent, invalid).IsSuccess(), "invalid save to absent parent fails");
    test.Expect(!std::filesystem::exists(absentParent.parent_path()), "encoding failure creates no parent directory");

    test.Expect(PlayerSaveFileStore::Save({}, original).Contains(PlayerSaveFileIssueCode::INVALID_PATH), "empty path rejects");
    test.Expect(PlayerSaveFileStore::Save(".", original).Contains(PlayerSaveFileIssueCode::INVALID_PATH), "dot filename rejects");
    test.Expect(PlayerSaveFileStore::Save("..", original).Contains(PlayerSaveFileIssueCode::INVALID_PATH), "dot-dot filename rejects");
    test.Expect(PlayerSaveFileStore::Save(root / "trailing" / "", original).Contains(PlayerSaveFileIssueCode::INVALID_PATH),
                "path with trailing separator rejects");
    test.Expect(PlayerSaveFileStore::Save(root.root_path(), original).Contains(PlayerSaveFileIssueCode::INVALID_PATH),
                "root-only path rejects");
    const auto parentFile = root / "parent-file"; WriteBytes(parentFile, "x");
    test.Expect(PlayerSaveFileStore::Save(parentFile / "player.save", original).Contains(PlayerSaveFileIssueCode::PARENT_PATH_IS_NOT_DIRECTORY),
                "parent path that is a file rejects");
    const auto directoryTarget = root / "directory-target"; std::filesystem::create_directory(directoryTarget);
    test.Expect(PlayerSaveFileStore::Save(directoryTarget, original).Contains(PlayerSaveFileIssueCode::TARGET_IS_DIRECTORY),
                "directory target rejects");

    PlayerSaveFileSaveResult relativeResult;
    {
        CurrentPathGuard currentPath(root);
        relativeResult = PlayerSaveFileStore::Save("relative.save", original);
    }
    test.Expect(relativeResult.IsSuccess() && std::filesystem::exists(root / "relative.save"), "relative filename works in caller-selected directory");

    const auto suffixTarget = root / "already.tmp";
    WriteBytes(std::filesystem::path(suffixTarget.string() + ".tmp"), "stale");
    test.Expect(PlayerSaveFileStore::Save(suffixTarget, original).IsSuccess(), "target already ending tmp uses appended tmp sidecar deterministically");
    test.Expect(!std::filesystem::exists(std::filesystem::path(suffixTarget.string() + ".tmp")), "appended temporary sidecar is cleaned");

    WriteBytes(temporary, "sidecar-before-invalid"); WriteBytes(backup, "backup-before-invalid");
    invalid = original; invalid.currentHealth = 0;
    const auto invalidWithSidecars = PlayerSaveFileStore::Save(target, invalid);
    test.Expect(!invalidWithSidecars.IsSuccess() && ReadBytes(temporary) == "sidecar-before-invalid" &&
                ReadBytes(backup) == "backup-before-invalid", "encoding failure leaves transaction sidecars byte-identical");
    std::filesystem::remove(temporary); std::filesystem::remove(backup);

    WriteBytes(temporary, "stale");
    test.Expect(PlayerSaveFileStore::Save(target, original).IsSuccess(), "stale temporary file is safely replaced");
    test.Expect(!std::filesystem::exists(temporary), "stale temporary removed after save");
    std::filesystem::create_directories(temporary / "child");
    WriteBytes(temporary / "child" / "block", "x");
    const std::string beforeTempFailure = ReadBytes(target);
    auto tempFailure = PlayerSaveFileStore::Save(target, replacement);
    test.Expect(!tempFailure.IsSuccess() && tempFailure.Contains(PlayerSaveFileIssueCode::TEMP_REMOVE_FAILED),
                "non-removable temporary path fails safely");
    test.ExpectEqual(ReadBytes(target), beforeTempFailure, "temporary preparation failure preserves target");
    std::filesystem::remove_all(temporary);

    for (bool nonempty : {false, true})
    {
        std::filesystem::create_directories(temporary);
        if (nonempty) WriteBytes(temporary / "block", "x");
        const std::string authoritative = ReadBytes(target);
        const auto directoryResult = PlayerSaveFileStore::Save(target, replacement);
        test.Expect(!directoryResult.IsSuccess() && directoryResult.Contains(PlayerSaveFileIssueCode::TEMP_REMOVE_FAILED),
                    std::string(nonempty ? "nonempty" : "empty") + " temporary directory rejects Save");
        test.Expect(std::filesystem::is_directory(temporary), "temporary sidecar directory is never deleted");
        test.ExpectEqual(ReadBytes(target), authoritative, "temporary sidecar directory preserves target");
        std::filesystem::remove_all(temporary);
    }

    for (bool nonempty : {false, true})
    {
        std::filesystem::create_directories(backup);
        if (nonempty) WriteBytes(backup / "block", "x");
        const std::string authoritative = ReadBytes(target);
        const auto directoryResult = PlayerSaveFileStore::Save(target, replacement);
        test.Expect(!directoryResult.IsSuccess() && directoryResult.Contains(PlayerSaveFileIssueCode::STALE_BACKUP_CLEANUP_FAILED),
                    std::string(nonempty ? "nonempty" : "empty") + " backup directory rejects Save");
        test.Expect(std::filesystem::is_directory(backup), "backup sidecar directory is never deleted");
        test.ExpectEqual(ReadBytes(target), authoritative, "backup sidecar directory preserves target");
        test.Expect(!std::filesystem::exists(temporary), "stale backup failure occurs before temporary writing");
        std::filesystem::remove_all(backup);
    }

    auto loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && loadResult.GetSaveData().has_value(), "valid file loads");
    test.Expect(loadResult.GetSaveData() && SameSave(*loadResult.GetSaveData(), original), "loaded persistent fields match exactly");
    for (const auto &sidecar : {temporary, backup})
    {
        std::filesystem::create_directory(sidecar);
        loadResult = PlayerSaveFileStore::Load(target);
        const auto expectedCode = sidecar == temporary ? PlayerSaveFileIssueCode::STALE_TEMP_CLEANUP_FAILED :
                                                        PlayerSaveFileIssueCode::STALE_BACKUP_CLEANUP_FAILED;
        test.Expect(loadResult.IsSuccess() && loadResult.Contains(expectedCode), "Load retains target with sidecar-directory warning");
        test.Expect(std::filesystem::is_directory(sidecar), "Load never deletes empty sidecar directory");
        std::filesystem::remove(sidecar);
    }
    const auto missing = root / "missing" / "player.save";
    loadResult = PlayerSaveFileStore::Load(missing);
    test.Expect(loadResult.IsNotFound() && loadResult.Contains(PlayerSaveFileIssueCode::FILE_NOT_FOUND), "missing load is distinct NOT_FOUND");
    test.Expect(!std::filesystem::exists(missing.parent_path()), "missing load creates no directories");
    const auto corrupt = root / "corrupt.save"; WriteBytes(corrupt, "");
    loadResult = PlayerSaveFileStore::Load(corrupt);
    test.Expect(!loadResult.IsSuccess() && !loadResult.IsNotFound() && loadResult.Contains(PlayerSaveFileIssueCode::DECODE_FAILED), "empty file fails decoding");
    test.Expect(loadResult.GetDecodeResult().has_value() && !loadResult.GetSaveData().has_value(), "corrupt load retains codec result and no save");
    WriteBytes(corrupt, "not-a-save\n");
    test.Expect(PlayerSaveFileStore::Load(corrupt).Contains(PlayerSaveFileIssueCode::DECODE_FAILED), "corrupt file fails");
    WriteBytes(corrupt, std::string(MAX_PLAYER_SAVE_TEXT_BYTES + 1, 'x'));
    test.Expect(PlayerSaveFileStore::Load(corrupt).Contains(PlayerSaveFileIssueCode::FILE_TOO_LARGE), "file one byte over limit rejects in reader");
    WriteBytes(corrupt, std::string(MAX_PLAYER_SAVE_TEXT_BYTES, 'x'));
    loadResult = PlayerSaveFileStore::Load(corrupt);
    test.Expect(!loadResult.Contains(PlayerSaveFileIssueCode::FILE_TOO_LARGE) && loadResult.Contains(PlayerSaveFileIssueCode::DECODE_FAILED),
                "file exactly at limit reaches decoder");
    WriteBytes(corrupt, canonical.substr(0, 20) + std::string(1, '\0') + canonical.substr(21));
    loadResult = PlayerSaveFileStore::Load(corrupt);
    test.Expect(loadResult.Contains(PlayerSaveFileIssueCode::DECODE_FAILED) &&
                loadResult.GetDecodeResult()->Contains(PlayerSaveTextIssueCode::EMBEDDED_NULL), "embedded NUL reaches codec");
    WriteBytes(corrupt, std::string(4095, 'x') + "\nmore");
    test.Expect(PlayerSaveFileStore::Load(corrupt).Contains(PlayerSaveFileIssueCode::DECODE_FAILED), "chunk-boundary bytes read completely into decoder");

    std::string crlf; for (char value : canonical) { if (value == '\n') crlf.push_back('\r'); crlf.push_back(value); }
    WriteBytes(corrupt, crlf);
    test.Expect(PlayerSaveFileStore::Load(corrupt).IsSuccess(), "CRLF valid file loads");
    WriteBytes(corrupt, canonical.substr(0, canonical.size() - 1));
    test.Expect(PlayerSaveFileStore::Load(corrupt).IsSuccess(), "valid file without final newline loads");
    std::string unsupportedVersion = canonical;
    const std::size_t versionPosition = unsupportedVersion.find("version=2");
    test.Expect(versionPosition != std::string::npos,
                "unsupported-version fixture finds canonical version record");
    if (versionPosition != std::string::npos)
        unsupportedVersion.replace(versionPosition, 9, "version=3");
    WriteBytes(corrupt, unsupportedVersion);
    test.Expect(PlayerSaveFileStore::Load(corrupt).Contains(PlayerSaveFileIssueCode::DECODE_FAILED), "unsupported file version fails through codec");

    // Existing target is authoritative over both transaction artifacts.
    WriteBytes(target, canonical); WriteBytes(temporary, replacementText);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && SameSave(*loadResult.GetSaveData(), original) && !std::filesystem::exists(temporary),
                "target alone remains authoritative over stale temporary");
    WriteBytes(backup, replacementText);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && SameSave(*loadResult.GetSaveData(), original) && !std::filesystem::exists(backup),
                "target alone remains authoritative over stale backup");
    WriteBytes(target, canonical); WriteBytes(temporary, "stale-temp"); WriteBytes(backup, replacementText);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && SameSave(*loadResult.GetSaveData(), original), "existing target remains authoritative over sidecars");
    test.Expect(!std::filesystem::exists(temporary) && !std::filesystem::exists(backup), "load cleans stale sidecars beside target");
    WriteBytes(target, "corrupt"); WriteBytes(backup, canonical);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.Contains(PlayerSaveFileIssueCode::DECODE_FAILED), "corrupt existing target does not fall back to backup");
    test.Expect(!std::filesystem::exists(backup), "stale backup is discarded when target exists");

    std::filesystem::remove(target); WriteBytes(backup, canonical);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && std::filesystem::exists(target) && !std::filesystem::exists(backup), "backup alone restores and loads target");
    std::filesystem::remove(target); WriteBytes(backup, canonical); WriteBytes(temporary, replacementText);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && SameSave(*loadResult.GetSaveData(), original), "backup wins over temporary during recovery");
    test.Expect(!std::filesystem::exists(temporary) && !std::filesystem::exists(backup), "backup recovery consumes sidecars");

    std::filesystem::remove(target); WriteBytes(temporary, replacementText);
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && SameSave(*loadResult.GetSaveData(), replacement), "valid temporary alone is promoted and loaded");
    test.Expect(std::filesystem::exists(target) && !std::filesystem::exists(temporary), "temporary promotion creates target and removes sidecar");
    std::filesystem::remove(target); WriteBytes(temporary, "invalid");
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsNotFound() && loadResult.Contains(PlayerSaveFileIssueCode::RECOVERY_INVALID_TEMP_DISCARDED),
                "invalid temporary is discarded and load becomes NOT_FOUND");
    test.Expect(!std::filesystem::exists(temporary), "invalid recovered temporary disappears");
    test.Expect(PlayerSaveFileStore::Save(target, original).IsSuccess(), "valid save succeeds after invalid temporary cleanup");

    // A non-empty backup directory induces a portable cleanup warning on load.
    std::filesystem::create_directories(backup / "child"); WriteBytes(backup / "child" / "block", "x");
    loadResult = PlayerSaveFileStore::Load(target);
    test.Expect(loadResult.IsSuccess() && loadResult.Contains(PlayerSaveFileIssueCode::STALE_BACKUP_CLEANUP_FAILED),
                "load success may retain machine-readable cleanup warning");
    test.Expect(!loadResult.GetIssues().empty() && loadResult.GetIssues().front().severity == PlayerSaveFileIssueSeverity::WARNING,
                "cleanup warning does not erase loaded state");
    std::filesystem::remove_all(backup);

    std::filesystem::create_directories(backup / "child"); WriteBytes(backup / "child" / "block", "x");
    const std::string beforeReplacementFailure = ReadBytes(target);
    auto replacementFailure = PlayerSaveFileStore::Save(target, replacement);
    test.Expect(!replacementFailure.IsSuccess() && replacementFailure.Contains(PlayerSaveFileIssueCode::STALE_BACKUP_CLEANUP_FAILED),
                "portable stale-backup failure is structured before replacement");
    test.ExpectEqual(ReadBytes(target), beforeReplacementFailure, "replacement-stage failure preserves authoritative target");
    test.Expect(!std::filesystem::exists(temporary), "replacement-stage failure cleans temporary file");
    std::filesystem::remove_all(backup);

    std::filesystem::remove(target); WriteBytes(temporary, "invalid");
    auto saveAfterInvalidTemporary = PlayerSaveFileStore::Save(target, original);
    test.Expect(saveAfterInvalidTemporary.IsSuccess() &&
                saveAfterInvalidTemporary.Contains(PlayerSaveFileIssueCode::RECOVERY_INVALID_TEMP_DISCARDED),
                "Save discards invalid interrupted first save and commits new data");

    Player runtimePlayer(50, PlayerInitializationMode::EMPTY);
    runtimePlayer.GetPosition().SetPosition(19, -8);
    auto runtimeSlots = runtimePlayer.GetInventory().GetSlots();
    runtimeSlots[4].SetItem(ItemType::COINS, 12345);
    runtimePlayer.GetInventory().TryReplaceSlotsAtomically(runtimeSlots);
    runtimePlayer.GetSkills().AddXP(SkillType::ATTACK, 777);
    const PlayerSaveData captured = PlayerSaveState::Capture(runtimePlayer);
    const auto roundTripTarget = root / "round-trip.save";
    test.Expect(PlayerSaveFileStore::Save(roundTripTarget, captured).IsSuccess(), "Player capture saves to file");
    auto roundTripLoad = PlayerSaveFileStore::Load(roundTripTarget);
    PlayerSaveValidationReport restoreReport;
    auto restored = PlayerSaveState::TryCreatePlayer(999, *roundTripLoad.GetSaveData(), restoreReport);
    test.Expect(restored && restored->GetID() == 999, "loaded data reconstructs with separate runtime ID");
    test.Expect(restored && SameSave(PlayerSaveState::Capture(*restored), captured), "full stable Player file round-trip preserves persistent state");
    test.Expect(restored && restored->GetStatusEffectManager().GetActiveEffects().empty(), "status effects remain excluded");

    Player boosted(51, PlayerInitializationMode::EMPTY);
    const StatusEffectDefinition boost{StatusEffectType::MAX_HEALTH_BOOST, 10, StatusEffectModifiers{0,0,0,50}};
    boosted.GetStatusEffectManager().Apply(boost, 0); boosted.RefreshDerivedState(); boosted.Heal(50);
    const PlayerSaveData boostedCapture = PlayerSaveState::Capture(boosted);
    const auto boostedTarget = root / "boosted.save";
    PlayerSaveFileStore::Save(boostedTarget, boostedCapture);
    auto boostedLoad = PlayerSaveFileStore::Load(boostedTarget);
    test.Expect(ReadBytes(boostedTarget).find("current_health=150\n") != std::string::npos,
                "temporary over-maximum health is stored verbatim in canonical file");
    test.Expect(boostedLoad.IsSuccess() && boostedLoad.GetSaveData()->currentHealth == 150, "file load preserves temporary over-maximum captured health");
    auto normalized = PlayerSaveState::TryCreatePlayer(1000, *boostedLoad.GetSaveData(), restoreReport);
    test.Expect(normalized && normalized->GetCurrentHealth() == 100, "Player reconstruction normalizes temporary over-maximum health");
    test.Expect(ReadBytes(boostedTarget).find("current_health=150\n") != std::string::npos,
                "Load and reconstruction do not rewrite stored temporary health");

    return test.Finish();
}
