#include "TestSupport.h"

#include "../src/Item/ItemDatabase.h"
#include "../src/Persistence/PersistenceTokenCodec.h"
#include "../src/Persistence/PlayerSaveState.h"
#include "../src/Persistence/PlayerSaveTextCodec.h"
#include "../src/Player/Player.h"
#include "../src/StatusEffect/StatusEffectDefinition.h"

#include <algorithm>
#include <array>
#include <climits>
#include <set>
#include <string>
#include <vector>

namespace
{
    PlayerSaveData ValidSave()
    {
        Player player(7, PlayerInitializationMode::EMPTY);
        PlayerSaveData save = PlayerSaveState::Capture(player);
        save.positionX = 3;
        save.positionY = 2;
        save.currentHealth = 87;
        save.inventorySlots[0] = {ItemType::BRONZE_AXE, 1};
        save.inventorySlots[1] = {ItemType::COINS, 150};
        save.skills[2].xp = 1200;
        save.skills[3].xp = 500;
        save.skills[4].xp = 300;
        save.equipment[3].itemType = ItemType::BRONZE_SWORD;
        save.equipment[4].itemType = ItemType::WOODEN_SHIELD;
        return save;
    }

    std::string Encode(const PlayerSaveData &save)
    {
        std::string text;
        PlayerSaveValidationReport report;
        if (!PlayerSaveTextCodec::TryEncode(save, text, report)) return {};
        return text;
    }

    std::string Replace(std::string text, std::string_view from, std::string_view to)
    {
        const auto position = text.find(from);
        if (position != std::string::npos) text.replace(position, from.size(), to);
        return text;
    }

    void Rejects(TestContext &test, const std::string &text, PlayerSaveTextIssueCode code, const std::string &name)
    {
        try
        {
            const auto result = PlayerSaveTextCodec::Decode(text);
            test.Expect(!result.IsSuccess(), name + " rejects");
            test.Expect(result.Contains(code), name + " reports expected issue");
            test.Expect(!result.GetSaveData().has_value(), name + " exposes no save data");
        }
        catch (...)
        {
            test.Expect(false, name + " does not throw");
        }
    }

    void EncodeRejectsUnchanged(TestContext &test, const PlayerSaveData &save, const std::string &name)
    {
        std::string output = "existing-output";
        PlayerSaveValidationReport report;
        test.Expect(!PlayerSaveTextCodec::TryEncode(save, output, report), name + " fails encoding");
        test.ExpectEqual(output, std::string("existing-output"), name + " preserves caller output");
        test.Expect(!report.IsValid(), name + " retains validation failure");
    }
}

int main()
{
    TestContext test;
    const std::array<std::pair<ItemType, std::string_view>, 22> itemTokens{{
        {ItemType::NONE,"NONE"},{ItemType::LOG,"LOG"},{ItemType::OAK_LOG,"OAK_LOG"},
        {ItemType::WILLOW_LOG,"WILLOW_LOG"},{ItemType::COPPER_ORE,"COPPER_ORE"},
        {ItemType::TIN_ORE,"TIN_ORE"},{ItemType::IRON_ORE,"IRON_ORE"},{ItemType::COAL,"COAL"},
        {ItemType::BRONZE_BAR,"BRONZE_BAR"},{ItemType::IRON_BAR,"IRON_BAR"},{ItemType::STEEL_BAR,"STEEL_BAR"},
        {ItemType::COINS,"COINS"},{ItemType::COOKED_MEAT,"COOKED_MEAT"},{ItemType::BRONZE_AXE,"BRONZE_AXE"},
        {ItemType::IRON_AXE,"IRON_AXE"},{ItemType::STEEL_AXE,"STEEL_AXE"},{ItemType::BRONZE_PICKAXE,"BRONZE_PICKAXE"},
        {ItemType::IRON_PICKAXE,"IRON_PICKAXE"},{ItemType::STEEL_PICKAXE,"STEEL_PICKAXE"},
        {ItemType::BRONZE_SWORD,"BRONZE_SWORD"},{ItemType::DEVELOPER_GODSWORD,"DEVELOPER_GODSWORD"},
        {ItemType::WOODEN_SHIELD,"WOODEN_SHIELD"}}};
    std::set<std::string_view> uniqueItems;
    std::set<ItemType> mappedItems;
    for (const auto &[type, expected] : itemTokens)
    {
        const auto token = PersistenceTokenCodec::TryGetItemToken(type);
        test.Expect(token && *token == expected, "item has explicit stable token");
        test.Expect(PersistenceTokenCodec::TryParseItemToken(expected) == type, "item token round-trips");
        uniqueItems.insert(expected);
        if (type != ItemType::NONE) mappedItems.insert(type);
    }
    test.ExpectEqual(uniqueItems.size(), itemTokens.size(), "item tokens are unique");
    test.ExpectEqual(ItemDatabase::GetAllItemTypes().size() + 1, itemTokens.size(), "all registered items plus NONE mapped");
    const std::set<ItemType> registeredItems(
        ItemDatabase::GetAllItemTypes().begin(), ItemDatabase::GetAllItemTypes().end());
    test.Expect(mappedItems == registeredItems, "explicit mapped item set exactly equals ItemDatabase registrations");
    test.Expect(!PersistenceTokenCodec::TryGetItemToken(static_cast<ItemType>(999)), "unknown item cast rejects");
    test.Expect(!PersistenceTokenCodec::TryParseItemToken("coins"), "lowercase item rejects");
    test.Expect(!PersistenceTokenCodec::TryParseItemToken("COINS "), "spaced item rejects");

    const std::array<std::pair<SkillType,std::string_view>,5> skills{{
        {SkillType::ATTACK,"ATTACK"},{SkillType::DEFENCE,"DEFENCE"},{SkillType::WOODCUTTING,"WOODCUTTING"},
        {SkillType::MINING,"MINING"},{SkillType::SMITHING,"SMITHING"}}};
    for (const auto &[type, token] : skills)
    { test.Expect(PersistenceTokenCodec::TryGetSkillToken(type) == token, "skill explicit token"); test.Expect(PersistenceTokenCodec::TryParseSkillToken(token) == type, "skill round-trip"); }
    test.Expect(!PersistenceTokenCodec::TryGetSkillToken(SkillType::NONE), "skill NONE rejects");
    test.Expect(!PersistenceTokenCodec::TryGetSkillToken(static_cast<SkillType>(999)), "unknown skill cast rejects");
    test.Expect(!PersistenceTokenCodec::TryParseSkillToken("attack"), "wrong-case skill rejects");
    test.Expect(!PersistenceTokenCodec::TryParseSkillToken("ATTACK ") &&
                !PersistenceTokenCodec::TryParseSkillToken(" ATTACK"), "skill whitespace rejects");

    const std::array<std::pair<EquipmentSlotType,std::string_view>,5> slots{{
        {EquipmentSlotType::HEAD,"HEAD"},{EquipmentSlotType::BODY,"BODY"},{EquipmentSlotType::LEGS,"LEGS"},
        {EquipmentSlotType::WEAPON,"WEAPON"},{EquipmentSlotType::SHIELD,"SHIELD"}}};
    for (const auto &[type, token] : slots)
    { test.Expect(PersistenceTokenCodec::TryGetEquipmentSlotToken(type) == token, "slot explicit token"); test.Expect(PersistenceTokenCodec::TryParseEquipmentSlotToken(token) == type, "slot round-trip"); }
    test.Expect(!PersistenceTokenCodec::TryGetEquipmentSlotToken(EquipmentSlotType::NONE), "slot NONE rejects");
    test.Expect(!PersistenceTokenCodec::TryGetEquipmentSlotToken(EquipmentSlotType::COUNT), "slot COUNT rejects");
    test.Expect(!PersistenceTokenCodec::TryParseEquipmentSlotToken("weapon"), "wrong-case slot rejects");
    test.Expect(!PersistenceTokenCodec::TryGetEquipmentSlotToken(static_cast<EquipmentSlotType>(999)), "unknown slot cast rejects");
    test.Expect(!PersistenceTokenCodec::TryParseEquipmentSlotToken("WEAPON ") &&
                !PersistenceTokenCodec::TryParseEquipmentSlotToken(" WEAPON"), "slot whitespace rejects");

    PlayerSaveData save = ValidSave();
    const std::string canonical = Encode(save);
    std::string golden = "GAMEENGINE_PLAYER_SAVE\nversion=1\nposition_x=3\nposition_y=2\ncurrent_health=87\ninventory_count=28\n";
    golden += "inventory.0=BRONZE_AXE,1\ninventory.1=COINS,150\n";
    for (int i = 2; i < 28; ++i) golden += "inventory." + std::to_string(i) + "=NONE,0\n";
    golden += "skill_count=5\nskill.ATTACK=0\nskill.DEFENCE=0\nskill.WOODCUTTING=1200\nskill.MINING=500\nskill.SMITHING=300\n";
    golden += "equipment_count=5\nequipment.HEAD=NONE\nequipment.BODY=NONE\nequipment.LEGS=NONE\nequipment.WEAPON=BRONZE_SWORD\nequipment.SHIELD=WOODEN_SHIELD\nEND_GAMEENGINE_PLAYER_SAVE\n";
    test.ExpectEqual(canonical, golden, "complete canonical golden output");
    test.Expect(!canonical.empty() && canonical.back() == '\n', "canonical output has final LF");
    test.Expect(canonical.find('\r') == std::string::npos, "canonical output uses LF only");
    test.Expect(canonical.find("entity") == std::string::npos, "runtime entity ID excluded");
    test.ExpectEqual(Encode(save), canonical, "repeated encoding byte-identical");
    std::reverse(save.skills.begin(), save.skills.end()); std::reverse(save.equipment.begin(), save.equipment.end());
    test.ExpectEqual(Encode(save), canonical, "reordered vectors encode identically");
    save.currentHealth = 0; std::string unchanged = "sentinel"; PlayerSaveValidationReport invalidReport;
    test.Expect(!PlayerSaveTextCodec::TryEncode(save, unchanged, invalidReport), "invalid save fails encoding");
    test.ExpectEqual(unchanged, std::string("sentinel"), "failed encoding leaves output unchanged");
    PlayerSaveData invalid = ValidSave(); invalid.version = 2; EncodeRejectsUnchanged(test, invalid, "unsupported version");
    invalid = ValidSave(); invalid.inventorySlots[0] = {ItemType::NONE, 1}; EncodeRejectsUnchanged(test, invalid, "invalid inventory");
    invalid = ValidSave(); invalid.skills.pop_back(); EncodeRejectsUnchanged(test, invalid, "missing skill");
    invalid = ValidSave(); invalid.skills[1].skillType = SkillType::ATTACK; EncodeRejectsUnchanged(test, invalid, "duplicate skill");
    invalid = ValidSave(); invalid.equipment[0].itemType = ItemType::BRONZE_SWORD; EncodeRejectsUnchanged(test, invalid, "invalid equipment slot");

    auto decoded = PlayerSaveTextCodec::Decode(canonical);
    test.Expect(decoded.IsSuccess() && decoded.GetIssues().empty(), "canonical text decodes with no issues");
    test.Expect(decoded.GetSaveData() && decoded.GetSaveData()->positionX == 3 && decoded.GetSaveData()->inventorySlots[1].quantity == 150, "decoded values exact");
    bool exactInventory = decoded.GetSaveData().has_value();
    for (std::size_t i = 0; exactInventory && i < Inventory::SlotCount; ++i)
    {
        const SavedInventorySlot &slot = decoded.GetSaveData()->inventorySlots[i];
        exactInventory = i == 0 ? slot.itemType == ItemType::BRONZE_AXE && slot.quantity == 1 :
            i == 1 ? slot.itemType == ItemType::COINS && slot.quantity == 150 :
            slot.itemType == ItemType::NONE && slot.quantity == 0;
    }
    test.Expect(exactInventory, "all 28 decoded inventory indexes remain exact");
    test.ExpectEqual(Encode(*decoded.GetSaveData()), canonical, "encode decode encode byte-identical");
    std::string crlf; for (char c : canonical) { if (c == '\n') crlf.push_back('\r'); crlf.push_back(c); }
    test.Expect(PlayerSaveTextCodec::Decode(crlf).IsSuccess(), "CRLF decodes");
    test.Expect(PlayerSaveTextCodec::Decode(canonical.substr(0, canonical.size()-1)).IsSuccess(), "missing final newline decodes");
    std::string mixedNewlines = canonical;
    mixedNewlines.replace(mixedNewlines.find('\n'), 1, "\r\n");
    test.Expect(PlayerSaveTextCodec::Decode(mixedNewlines).IsSuccess(), "per-line LF and CRLF styles may be mixed deterministically");
    std::string reordered = Replace(canonical,
        "skill.ATTACK=0\nskill.DEFENCE=0\nskill.WOODCUTTING=1200\nskill.MINING=500\nskill.SMITHING=300\n",
        "skill.SMITHING=300\nskill.MINING=500\nskill.WOODCUTTING=1200\nskill.DEFENCE=0\nskill.ATTACK=0\n");
    auto reorderResult = PlayerSaveTextCodec::Decode(reordered);
    test.Expect(reorderResult.IsSuccess(), "noncanonical middle ordering decodes");
    test.Expect(reorderResult.IsSuccess() && Encode(*reorderResult.GetSaveData()) == canonical, "re-encoding canonicalizes order");
    std::vector<std::string> canonicalLines;
    for (std::size_t start = 0; start < canonical.size();)
    {
        const std::size_t end = canonical.find('\n', start);
        if (end == std::string::npos) break;
        canonicalLines.push_back(canonical.substr(start, end - start));
        start = end + 1;
    }
    std::reverse(canonicalLines.begin() + 1, canonicalLines.end() - 1);
    std::string fullyReorderedCrlf;
    for (std::size_t i = 0; i < canonicalLines.size(); ++i)
    {
        if (i != 0) fullyReorderedCrlf += "\r\n";
        fullyReorderedCrlf += canonicalLines[i];
    }
    const auto fullCanonicalization = PlayerSaveTextCodec::Decode(fullyReorderedCrlf);
    test.Expect(fullCanonicalization.IsSuccess(), "reordered scalars inventory skills and equipment decode from CRLF without final newline");
    const std::string recanonicalized = fullCanonicalization.IsSuccess() ? Encode(*fullCanonicalization.GetSaveData()) : std::string{};
    test.ExpectEqual(recanonicalized, canonical, "fully reordered input canonicalizes to exact LF golden text");
    const auto secondCanonicalization = PlayerSaveTextCodec::Decode(recanonicalized);
    test.Expect(secondCanonicalization.IsSuccess() && Encode(*secondCanonicalization.GetSaveData()) == canonical,
                "second canonical decode encode remains byte-identical");

    Rejects(test, "", PlayerSaveTextIssueCode::EMPTY_INPUT, "empty input");
    Rejects(test, "\xEF\xBB\xBF" + canonical, PlayerSaveTextIssueCode::INVALID_HEADER, "BOM");
    Rejects(test, Replace(canonical,"GAMEENGINE_PLAYER_SAVE","gameengine_player_save"), PlayerSaveTextIssueCode::INVALID_HEADER, "wrong header case");
    Rejects(test, Replace(canonical,"END_GAMEENGINE_PLAYER_SAVE\n",""), PlayerSaveTextIssueCode::INVALID_END_MARKER, "missing end");
    Rejects(test, Replace(canonical,"END_GAMEENGINE_PLAYER_SAVE","end_gameengine_player_save"), PlayerSaveTextIssueCode::INVALID_END_MARKER, "wrong end-marker case");
    Rejects(test, Replace(canonical,"END_GAMEENGINE_PLAYER_SAVE","END_GAMEENGINE_PLAYER_SAVE "), PlayerSaveTextIssueCode::INVALID_END_MARKER, "spaced end marker");
    Rejects(test, canonical + "x\n", PlayerSaveTextIssueCode::DATA_AFTER_END_MARKER, "data after end");
    Rejects(test, Replace(canonical,"position_x=3\n","GAMEENGINE_PLAYER_SAVE\nposition_x=3\n"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "repeated middle header");
    Rejects(test, Replace(canonical,"position_x=3\n","END_GAMEENGINE_PLAYER_SAVE\nposition_x=3\n"), PlayerSaveTextIssueCode::DATA_AFTER_END_MARKER, "early end marker");
    Rejects(test, Replace(canonical,"position_x=3\n","END_GAMEENGINE_PLAYER_SAVE\nEND_GAMEENGINE_PLAYER_SAVE\nposition_x=3\n"), PlayerSaveTextIssueCode::DATA_AFTER_END_MARKER, "multiple end markers");
    Rejects(test, "\n" + canonical, PlayerSaveTextIssueCode::INVALID_HEADER, "blank before header");
    Rejects(test, Replace(canonical,"position_x=3\n","position_x=3\n\n"), PlayerSaveTextIssueCode::EMPTY_LINE, "blank data line");
    Rejects(test, canonical + "\n", PlayerSaveTextIssueCode::DATA_AFTER_END_MARKER, "blank after end");
    Rejects(test, canonical.substr(0,3) + std::string(1,'\0') + canonical.substr(4), PlayerSaveTextIssueCode::EMBEDDED_NULL, "embedded NUL");
    Rejects(test, Replace(canonical,"position_x=3\n","position_x=3\rposition_y=2\n"), PlayerSaveTextIssueCode::INVALID_LINE_ENDING, "bare CR");
    Rejects(test, Replace(canonical,"position_x=3\n","position_x=3\r\r\n"), PlayerSaveTextIssueCode::INVALID_LINE_ENDING, "double CR");
    Rejects(test, std::string(MAX_PLAYER_SAVE_TEXT_BYTES + 1, 'x'), PlayerSaveTextIssueCode::INPUT_TOO_LARGE, "oversized input");
    auto exactBytes = PlayerSaveTextCodec::Decode(std::string(MAX_PLAYER_SAVE_TEXT_BYTES, 'x'));
    test.Expect(!exactBytes.Contains(PlayerSaveTextIssueCode::INPUT_TOO_LARGE), "exact byte limit passes byte-size gate");
    Rejects(test, "GAMEENGINE_PLAYER_SAVE\n" + std::string(MAX_PLAYER_SAVE_TEXT_LINE_LENGTH+1,'x') + "\nEND_GAMEENGINE_PLAYER_SAVE", PlayerSaveTextIssueCode::LINE_TOO_LONG, "long line");
    auto exactLine = PlayerSaveTextCodec::Decode("GAMEENGINE_PLAYER_SAVE\n" + std::string(MAX_PLAYER_SAVE_TEXT_LINE_LENGTH,'x') + "\nEND_GAMEENGINE_PLAYER_SAVE");
    test.Expect(!exactLine.Contains(PlayerSaveTextIssueCode::LINE_TOO_LONG), "exact line-length limit passes line-size gate");
    std::string many = "GAMEENGINE_PLAYER_SAVE\n"; for (std::size_t i=0;i<MAX_PLAYER_SAVE_TEXT_LINES;++i) many += "x\n";
    Rejects(test, many, PlayerSaveTextIssueCode::TOO_MANY_LINES, "too many lines");
    std::string exactLines = "GAMEENGINE_PLAYER_SAVE\n";
    for (std::size_t i=0;i<MAX_PLAYER_SAVE_TEXT_LINES-2;++i) exactLines += "x\n";
    exactLines += "END_GAMEENGINE_PLAYER_SAVE";
    test.Expect(!PlayerSaveTextCodec::Decode(exactLines).Contains(PlayerSaveTextIssueCode::TOO_MANY_LINES), "exact line-count limit passes line gate");

    Rejects(test, Replace(canonical,"position_x=3","position_x"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "missing equals");
    Rejects(test, Replace(canonical,"position_x=3","position_x==3"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "extra equals");
    Rejects(test, Replace(canonical,"version=1","version==1"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "malformed version extra equals");
    Rejects(test, Replace(canonical,"version=1","version=1=2"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "malformed version value separator");
    Rejects(test, Replace(canonical,"version=1","version="), PlayerSaveTextIssueCode::MALFORMED_RECORD, "empty version value");
    Rejects(test, Replace(canonical,"version=1","=1"), PlayerSaveTextIssueCode::MISSING_FIELD, "empty version key");
    Rejects(test, Replace(canonical,"position_x=3"," position_x=3"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "leading space");
    Rejects(test, Replace(canonical,"position_x=3","position_x=3 "), PlayerSaveTextIssueCode::MALFORMED_RECORD, "trailing space");
    Rejects(test, Replace(canonical,"position_x=3","mystery=3"), PlayerSaveTextIssueCode::UNKNOWN_FIELD, "unknown field");
    Rejects(test, Replace(canonical,"position_x=3","Position_x=3"), PlayerSaveTextIssueCode::UNKNOWN_FIELD, "wrong-case scalar");
    Rejects(test, Replace(canonical,"position_x=3","position_x_extra=3"), PlayerSaveTextIssueCode::UNKNOWN_FIELD, "similar-prefix scalar");
    Rejects(test, Replace(canonical,"position_x=3\n","position_x=3\nposition_x=4\n"), PlayerSaveTextIssueCode::DUPLICATE_FIELD, "duplicate scalar");
    Rejects(test, Replace(canonical,"current_health=87\n",""), PlayerSaveTextIssueCode::MISSING_FIELD, "missing scalar");
    Rejects(test, Replace(canonical,"position_x=3","position_x=\"3\""), PlayerSaveTextIssueCode::INVALID_INTEGER, "quoted integer");
    Rejects(test, Replace(canonical,"position_x=3","position_x=+3"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "leading plus");
    Rejects(test, Replace(canonical,"position_x=3","position_x=03"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "leading zero");
    Rejects(test, Replace(canonical,"position_x=3","position_x=-0"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "negative zero");
    Rejects(test, Replace(canonical,"position_x=3","position_x=-01"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "negative leading zero");
    Rejects(test, Replace(canonical,"position_x=3","position_x=1.0"), PlayerSaveTextIssueCode::INVALID_INTEGER, "decimal integer");
    Rejects(test, Replace(canonical,"position_x=3","position_x=1e3"), PlayerSaveTextIssueCode::INVALID_INTEGER, "exponent integer");
    Rejects(test, Replace(canonical,"position_x=3","position_x=0x10"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "hex integer");
    Rejects(test, Replace(canonical,"position_x=3","position_x=2147483648"), PlayerSaveTextIssueCode::INTEGER_OUT_OF_RANGE, "integer overflow");
    test.Expect(PlayerSaveTextCodec::Decode(Replace(canonical,"position_x=3","position_x=-2147483648")).IsSuccess(), "INT_MIN position decodes");
    test.Expect(PlayerSaveTextCodec::Decode(Replace(canonical,"position_x=3","position_x=2147483647")).IsSuccess(), "INT_MAX position decodes");
    Rejects(test, Replace(canonical,"version=1","version=2"), PlayerSaveTextIssueCode::UNSUPPORTED_VERSION, "unsupported version");
    Rejects(test, Replace(canonical,"version=1","version=0"), PlayerSaveTextIssueCode::UNSUPPORTED_VERSION, "zero version");
    Rejects(test, Replace(canonical,"version=1","version=-1"), PlayerSaveTextIssueCode::UNSUPPORTED_VERSION, "negative version");
    Rejects(test, Replace(canonical,"version=1","version=2147483647"), PlayerSaveTextIssueCode::UNSUPPORTED_VERSION, "future INT_MAX version");
    Rejects(test, Replace(canonical,"inventory_count=28","inventory_count=999999999"), PlayerSaveTextIssueCode::INVALID_COUNT, "attacker count");
    Rejects(test, Replace(canonical,"skill_count=5","skill_count=05"), PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER, "noncanonical count");
    Rejects(test, Replace(canonical,"equipment_count=5\n",""), PlayerSaveTextIssueCode::MISSING_FIELD, "missing count");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.00=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "noncanonical index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.28=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "out of range index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.-1=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "negative inventory index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.+1=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "plus inventory index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "empty inventory index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.1.extra=BRONZE_AXE,1"), PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, "inventory index suffix");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=UNKNOWN,1"), PlayerSaveTextIssueCode::UNKNOWN_ITEM_TOKEN, "unknown inventory item");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1\n","inventory.0=BRONZE_AXE,1\ninventory.0=BRONZE_AXE,1\n"), PlayerSaveTextIssueCode::DUPLICATE_INVENTORY_INDEX, "duplicate inventory index");
    Rejects(test, Replace(canonical,"inventory.27=NONE,0\n",""), PlayerSaveTextIssueCode::MISSING_INVENTORY_INDEX, "missing inventory index");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=BRONZE_AXE"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "missing inventory comma");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=BRONZE_AXE,1,2"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "extra inventory comma");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=,1"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "empty inventory item");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=BRONZE_AXE,"), PlayerSaveTextIssueCode::MALFORMED_RECORD, "empty inventory quantity");
    Rejects(test, Replace(canonical,"inventory.2=NONE,0","inventory.2=NONE,1"), PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, "noncanonical empty inventory");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1","inventory.0=BRONZE_AXE,2"), PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, "nonstackable quantity");
    Rejects(test, Replace(canonical,"inventory.2=NONE,0","inventory.2=COINS,1"), PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, "duplicate stackable item");
    Rejects(test, Replace(canonical,"skill.ATTACK=0","skill.attack=0"), PlayerSaveTextIssueCode::UNKNOWN_SKILL_TOKEN, "lowercase skill");
    Rejects(test, Replace(canonical,"skill.ATTACK=0","skill.NONE=0"), PlayerSaveTextIssueCode::UNKNOWN_SKILL_TOKEN, "skill NONE");
    Rejects(test, Replace(canonical,"skill.ATTACK=0\n","skill.ATTACK=0\nskill.ATTACK=0\n"), PlayerSaveTextIssueCode::DUPLICATE_SKILL_RECORD, "duplicate skill record");
    for (const auto &[type, token] : skills)
    {
        const std::string record = "skill." + std::string(token) + "=" +
            (type == SkillType::WOODCUTTING ? "1200" : type == SkillType::MINING ? "500" : type == SkillType::SMITHING ? "300" : "0") + "\n";
        Rejects(test, Replace(canonical, record, ""), PlayerSaveTextIssueCode::MISSING_SKILL_RECORD,
                "missing skill " + std::string(token));
    }
    auto negativeXP = PlayerSaveTextCodec::Decode(Replace(canonical,"skill.ATTACK=0","skill.ATTACK=-1"));
    test.Expect(negativeXP.Contains(PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED) && negativeXP.GetValidationReport().Contains(PlayerSaveValidationCode::NEGATIVE_SKILL_XP), "semantic skill failure retains validation report");
    Rejects(test, Replace(canonical,"equipment.WEAPON=BRONZE_SWORD","equipment.weapon=BRONZE_SWORD"), PlayerSaveTextIssueCode::UNKNOWN_EQUIPMENT_SLOT_TOKEN, "lowercase equipment slot");
    Rejects(test, Replace(canonical,"equipment.HEAD=NONE","equipment.NONE=NONE"), PlayerSaveTextIssueCode::UNKNOWN_EQUIPMENT_SLOT_TOKEN, "equipment NONE slot");
    Rejects(test, Replace(canonical,"equipment.HEAD=NONE","equipment.COUNT=NONE"), PlayerSaveTextIssueCode::UNKNOWN_EQUIPMENT_SLOT_TOKEN, "equipment COUNT slot");
    Rejects(test, Replace(canonical,"equipment.HEAD=NONE\n","equipment.HEAD=NONE\nequipment.HEAD=NONE\n"), PlayerSaveTextIssueCode::DUPLICATE_EQUIPMENT_SLOT_RECORD, "duplicate equipment record");
    for (const auto &[type, token] : slots)
    {
        const std::string item = type == EquipmentSlotType::WEAPON ? "BRONZE_SWORD" :
            type == EquipmentSlotType::SHIELD ? "WOODEN_SHIELD" : "NONE";
        Rejects(test, Replace(canonical, "equipment." + std::string(token) + "=" + item + "\n", ""),
                PlayerSaveTextIssueCode::MISSING_EQUIPMENT_SLOT_RECORD, "missing equipment " + std::string(token));
    }
    Rejects(test, Replace(canonical,"equipment.HEAD=NONE","equipment.HEAD=BRONZE_SWORD"), PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, "wrong-slot equipment");
    std::string unmetRequirement = Replace(canonical,"equipment.WEAPON=BRONZE_SWORD","equipment.WEAPON=STEEL_AXE");
    unmetRequirement = Replace(unmetRequirement, "skill.WOODCUTTING=1200", "skill.WOODCUTTING=0");
    Rejects(test, unmetRequirement, PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, "unmet equipment requirement");
    test.Expect(PlayerSaveTextCodec::Decode(Replace(canonical,"skill.ATTACK=0","skill.ATTACK=2147483647")).IsSuccess(), "INT_MAX XP decodes exactly");
    test.Expect(PlayerSaveTextCodec::Decode(Replace(canonical,"inventory.1=COINS,150","inventory.1=COINS,2147483647")).IsSuccess(), "INT_MAX stack quantity decodes exactly");

    std::string nonAsciiHeader = canonical; nonAsciiHeader[0] = static_cast<char>(0xC3);
    Rejects(test, nonAsciiHeader, PlayerSaveTextIssueCode::INVALID_HEADER, "non-ASCII header");
    Rejects(test, Replace(canonical,"position_x=3",std::string("position_") + static_cast<char>(0xFF) + "=3"), PlayerSaveTextIssueCode::UNKNOWN_FIELD, "non-ASCII key");
    Rejects(test, Replace(canonical,"inventory.0=BRONZE_AXE,1",std::string("inventory.0=BRONZE_") + static_cast<char>(0xFF) + ",1"), PlayerSaveTextIssueCode::UNKNOWN_ITEM_TOKEN, "non-ASCII token");
    Rejects(test, Replace(canonical,"position_x=3",std::string("position_x=") + static_cast<char>(0xFF)), PlayerSaveTextIssueCode::INVALID_INTEGER, "non-ASCII integer");

    const auto badLine = PlayerSaveTextCodec::Decode(Replace(canonical,"position_x=3","position_x=+3"));
    test.Expect(!badLine.GetIssues().empty() && badLine.GetIssues().front().lineNumber == 3, "syntax issue reports one-based source line");

    Player original(40, PlayerInitializationMode::EMPTY);
    original.GetPosition().SetPosition(9, -4);
    auto originalSlots = original.GetInventory().GetSlots();
    originalSlots[0].SetItem(ItemType::COINS, 77);
    original.GetInventory().TryReplaceSlotsAtomically(originalSlots);
    PlayerSaveData captured = PlayerSaveState::Capture(original);
    const std::string first = Encode(captured);
    auto roundTrip = PlayerSaveTextCodec::Decode(first);
    PlayerSaveValidationReport restoreReport;
    auto restored = PlayerSaveState::TryCreatePlayer(999, *roundTrip.GetSaveData(), restoreReport);
    test.Expect(restored && restored->GetID() == 999, "decoded save reconstructs with separate runtime ID");
    test.Expect(restored && Encode(PlayerSaveState::Capture(*restored)) == first, "full Player round-trip is byte-identical");
    test.Expect(restored && restored->GetStatusEffectManager().GetActiveEffects().empty(), "status effects remain excluded");

    Player boosted(41, PlayerInitializationMode::EMPTY);
    const StatusEffectDefinition healthBoost{
        StatusEffectType::MAX_HEALTH_BOOST, 10, StatusEffectModifiers{0, 0, 0, 50}};
    test.Expect(boosted.GetStatusEffectManager().Apply(healthBoost, 0), "temporary health fixture applies effect");
    boosted.RefreshDerivedState();
    boosted.Heal(50);
    const PlayerSaveData boostedCapture = PlayerSaveState::Capture(boosted);
    const std::string boostedText = Encode(boostedCapture);
    const auto boostedDecode = PlayerSaveTextCodec::Decode(boostedText);
    test.Expect(boostedDecode.IsSuccess() && boostedDecode.GetSaveData()->currentHealth == 150,
                "codec preserves positive temporary over-base health exactly");
    PlayerSaveValidationReport boostedRestoreReport;
    auto normalizedPlayer = PlayerSaveState::TryCreatePlayer(1000, *boostedDecode.GetSaveData(), boostedRestoreReport);
    test.Expect(normalizedPlayer && normalizedPlayer->GetCurrentHealth() == 100,
                "reconstruction normalizes temporary over-maximum health");
    test.Expect(normalizedPlayer && Encode(PlayerSaveState::Capture(*normalizedPlayer)) != boostedText,
                "temporary-health normalization intentionally changes reconstructed encoding");

    return test.Finish();
}
