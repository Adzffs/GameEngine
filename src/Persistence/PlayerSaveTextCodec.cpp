#include "PlayerSaveTextCodec.h"

#include "PersistenceTokenCodec.h"
#include "PlayerSaveState.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <system_error>
#include <utility>

namespace
{
    constexpr std::string_view HEADER = "GAMEENGINE_PLAYER_SAVE";
    constexpr std::string_view END_MARKER = "END_GAMEENGINE_PLAYER_SAVE";

    void AppendInteger(std::string &output, int value)
    {
        std::array<char, 16> buffer{};
        const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        output.append(buffer.data(), result.ptr);
    }

    void AppendScalar(std::string &output, std::string_view key, int value)
    {
        output.append(key);
        output.push_back('=');
        AppendInteger(output, value);
        output.push_back('\n');
    }

    enum class IntegerResult { VALID, INVALID, NON_CANONICAL, OUT_OF_RANGE };

    IntegerResult ParseInteger(std::string_view value, int &output)
    {
        if (value.empty()) return IntegerResult::INVALID;
        if (value.front() == '+') return IntegerResult::NON_CANONICAL;
        if (value == "-0" || (value.size() > 1 && value.front() == '0') ||
            (value.size() > 2 && value[0] == '-' && value[1] == '0'))
            return IntegerResult::NON_CANONICAL;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), output);
        if (result.ec == std::errc::result_out_of_range) return IntegerResult::OUT_OF_RANGE;
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return IntegerResult::INVALID;
        return IntegerResult::VALID;
    }

    PlayerSaveTextIssueCode IntegerIssue(IntegerResult result)
    {
        if (result == IntegerResult::NON_CANONICAL) return PlayerSaveTextIssueCode::NON_CANONICAL_INTEGER;
        if (result == IntegerResult::OUT_OF_RANGE) return PlayerSaveTextIssueCode::INTEGER_OUT_OF_RANGE;
        return PlayerSaveTextIssueCode::INVALID_INTEGER;
    }

    struct LinesResult
    {
        std::vector<std::string_view> lines;
        PlayerSaveTextIssueCode issue = PlayerSaveTextIssueCode::EMPTY_INPUT;
        int line = -1;
        bool valid = false;
    };

    LinesResult SplitLines(std::string_view text)
    {
        LinesResult result;
        if (text.empty()) return result;
        if (text.size() > MAX_PLAYER_SAVE_TEXT_BYTES)
        { result.issue = PlayerSaveTextIssueCode::INPUT_TOO_LARGE; return result; }
        if (text.find('\0') != std::string_view::npos)
        { result.issue = PlayerSaveTextIssueCode::EMBEDDED_NULL; return result; }

        std::size_t start = 0;
        int lineNumber = 1;
        while (start < text.size())
        {
            const std::size_t newline = text.find('\n', start);
            const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
            std::string_view line = text.substr(start, end - start);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            if (line.find('\r') != std::string_view::npos)
            { result.issue = PlayerSaveTextIssueCode::INVALID_LINE_ENDING; result.line = lineNumber; return result; }
            if (line.size() > MAX_PLAYER_SAVE_TEXT_LINE_LENGTH)
            { result.issue = PlayerSaveTextIssueCode::LINE_TOO_LONG; result.line = lineNumber; return result; }
            result.lines.push_back(line);
            if (result.lines.size() > MAX_PLAYER_SAVE_TEXT_LINES)
            { result.issue = PlayerSaveTextIssueCode::TOO_MANY_LINES; result.line = lineNumber; return result; }
            if (newline == std::string_view::npos) break;
            start = newline + 1;
            ++lineNumber;
        }
        result.valid = true;
        return result;
    }

    int SkillIndex(SkillType type)
    {
        for (std::size_t i = 0; i < PERSISTED_SKILL_TYPES.size(); ++i)
            if (PERSISTED_SKILL_TYPES[i] == type) return static_cast<int>(i);
        return -1;
    }

    int SlotIndex(EquipmentSlotType type)
    {
        for (std::size_t i = 0; i < PERSISTED_EQUIPMENT_SLOT_TYPES.size(); ++i)
            if (PERSISTED_EQUIPMENT_SLOT_TYPES[i] == type) return static_cast<int>(i);
        return -1;
    }
    std::string_view QuestStateToken(QuestState s){switch(s){case QuestState::AVAILABLE:return "AVAILABLE";case QuestState::ACTIVE:return "ACTIVE";case QuestState::READY_TO_COMPLETE:return "READY_TO_COMPLETE";case QuestState::COMPLETED:return "COMPLETED";default:return "UNAVAILABLE";}}
    std::optional<QuestState> ParseQuestState(std::string_view s){if(s=="AVAILABLE")return QuestState::AVAILABLE;if(s=="ACTIVE")return QuestState::ACTIVE;if(s=="READY_TO_COMPLETE")return QuestState::READY_TO_COMPLETE;if(s=="COMPLETED")return QuestState::COMPLETED;return std::nullopt;}
}

bool PlayerSaveTextDecodeResult::IsSuccess() const { return issues.empty() && saveData.has_value(); }
const std::vector<PlayerSaveTextIssue> &PlayerSaveTextDecodeResult::GetIssues() const { return issues; }
bool PlayerSaveTextDecodeResult::Contains(PlayerSaveTextIssueCode code) const
{
    return std::any_of(issues.begin(), issues.end(), [code](const auto &issue) { return issue.code == code; });
}
const std::optional<PlayerSaveData> &PlayerSaveTextDecodeResult::GetSaveData() const { return saveData; }
const PlayerSaveValidationReport &PlayerSaveTextDecodeResult::GetValidationReport() const { return validationReport; }
void PlayerSaveTextDecodeResult::AddIssue(PlayerSaveTextIssueCode code, int lineNumber, std::string message)
{ issues.push_back({code, lineNumber, std::move(message)}); }

bool PlayerSaveTextCodec::TryEncode(const PlayerSaveData &saveData, std::string &output,
                                    PlayerSaveValidationReport &validationReport)
{
    if (saveData.version != CURRENT_PLAYER_SAVE_VERSION)
    {
        validationReport = PlayerSaveValidationReport{};
        validationReport.AddIssue(PlayerSaveValidationCode::UNSUPPORTED_VERSION,
                                  -1,
                                  "Only current-version player saves may be encoded");
        return false;
    }
    validationReport = PlayerSaveState::Validate(saveData);
    if (!validationReport.IsValid()) return false;

    std::array<const SavedSkillXP *, 5> skills{};
    for (const auto &skill : saveData.skills)
    {
        const int index = SkillIndex(skill.skillType);
        if (index < 0) return false;
        skills[static_cast<std::size_t>(index)] = &skill;
    }
    std::array<const SavedEquipmentSlot *, 5> equipment{};
    for (const auto &slot : saveData.equipment)
    {
        const int index = SlotIndex(slot.slotType);
        if (index < 0) return false;
        equipment[static_cast<std::size_t>(index)] = &slot;
    }

    std::string encoded;
    encoded.reserve(1024);
    encoded.append(HEADER).push_back('\n');
    AppendScalar(encoded, "version", saveData.version);
    AppendScalar(encoded, "position_x", saveData.positionX);
    AppendScalar(encoded, "position_y", saveData.positionY);
    AppendScalar(encoded, "current_health", saveData.currentHealth);
    AppendScalar(encoded, "inventory_count", static_cast<int>(Inventory::SlotCount));
    for (std::size_t i = 0; i < saveData.inventorySlots.size(); ++i)
    {
        const auto token = PersistenceTokenCodec::TryGetItemToken(saveData.inventorySlots[i].itemType);
        if (!token) return false;
        encoded.append("inventory."); AppendInteger(encoded, static_cast<int>(i));
        encoded.push_back('='); encoded.append(*token); encoded.push_back(',');
        AppendInteger(encoded, saveData.inventorySlots[i].quantity); encoded.push_back('\n');
    }
    AppendScalar(encoded, "skill_count", 5);
    for (std::size_t i = 0; i < skills.size(); ++i)
    {
        const auto token = PersistenceTokenCodec::TryGetSkillToken(PERSISTED_SKILL_TYPES[i]);
        encoded.append("skill.").append(*token).push_back('=');
        AppendInteger(encoded, skills[i]->xp); encoded.push_back('\n');
    }
    AppendScalar(encoded, "equipment_count", 5);
    for (std::size_t i = 0; i < equipment.size(); ++i)
    {
        const auto slotToken = PersistenceTokenCodec::TryGetEquipmentSlotToken(PERSISTED_EQUIPMENT_SLOT_TYPES[i]);
        const auto itemToken = PersistenceTokenCodec::TryGetItemToken(equipment[i]->itemType);
        if (!itemToken) return false;
        encoded.append("equipment.").append(*slotToken).push_back('=');
        encoded.append(*itemToken).push_back('\n');
    }
    encoded.append("quest.GATHERING_BASICS=").append(QuestStateToken(saveData.gatheringBasicsState)).push_back(','); AppendInteger(encoded,saveData.gatheringBasicsProgress); encoded.push_back('\n');
    encoded.append(END_MARKER).push_back('\n');
    output = std::move(encoded);
    return true;
}

PlayerSaveTextDecodeResult PlayerSaveTextCodec::Decode(std::string_view text)
{
    PlayerSaveTextDecodeResult result;
    LinesResult split = SplitLines(text);
    if (!split.valid)
    { result.AddIssue(split.issue, split.line, "Invalid player save text bounds or line structure"); return result; }
    const auto &lines = split.lines;
    if (lines.empty() || lines.front() != HEADER)
    { result.AddIssue(PlayerSaveTextIssueCode::INVALID_HEADER, lines.empty() ? -1 : 1, "Invalid player save header"); return result; }
    if (lines.size() < 2)
    { result.AddIssue(PlayerSaveTextIssueCode::INVALID_END_MARKER, -1, "Missing end marker"); return result; }
    std::size_t endIndex = lines.size();
    for (std::size_t i = 1; i < lines.size(); ++i)
        if (lines[i] == END_MARKER) { endIndex = i; break; }
    if (endIndex == lines.size())
    { result.AddIssue(PlayerSaveTextIssueCode::INVALID_END_MARKER, -1, "Missing end marker"); return result; }
    if (endIndex + 1 != lines.size())
    { result.AddIssue(PlayerSaveTextIssueCode::DATA_AFTER_END_MARKER, static_cast<int>(endIndex + 2), "Data after end marker"); return result; }

    int version = 0;
    int versionLine = -1;
    for (std::size_t i = 1; i < endIndex; ++i)
    {
        const std::string_view line = lines[i];
        const std::size_t equals = line.find('=');
        const bool versionKey = equals != std::string_view::npos &&
            line.substr(0, equals) == "version";
        const bool malformedVersionLike = line == "version" ||
            line.starts_with("version ") || line.starts_with("version\t");
        if (versionKey)
        {
            if (versionLine != -1)
            { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_FIELD, static_cast<int>(i + 1), "Duplicate version"); return result; }
            versionLine = static_cast<int>(i + 1);
            if (equals + 1 == line.size() || line.find('=', equals + 1) != std::string_view::npos)
            { result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD, versionLine, "Malformed version record"); return result; }
            const auto parsed = ParseInteger(line.substr(equals + 1), version);
            if (parsed != IntegerResult::VALID)
            { result.AddIssue(IntegerIssue(parsed), versionLine, "Invalid version integer"); return result; }
        }
        else if (malformedVersionLike)
        {
            result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD, static_cast<int>(i + 1), "Malformed version record");
            return result;
        }
    }
    if (versionLine == -1)
    { result.AddIssue(PlayerSaveTextIssueCode::MISSING_FIELD, -1, "Missing version"); return result; }
    if (version != 1 && version != CURRENT_PLAYER_SAVE_VERSION)
    { result.AddIssue(PlayerSaveTextIssueCode::UNSUPPORTED_VERSION, versionLine, "Unsupported player save version"); return result; }

    PlayerSaveData save;
    std::array<bool, 7> scalars{};
    std::array<bool, Inventory::SlotCount> inventorySeen{};
    std::array<bool, 5> skillsSeen{};
    std::array<bool, 5> equipmentSeen{};
    std::array<int, 5> skillXP{};
    std::array<ItemType, 5> equipmentItems{};
    bool questSeen=false;
    auto addInteger = [&](std::string_view value, int line, int &target)
    {
        const auto parsed = ParseInteger(value, target);
        if (parsed != IntegerResult::VALID) result.AddIssue(IntegerIssue(parsed), line, "Invalid integer");
        return parsed == IntegerResult::VALID;
    };
    auto scalar = [&](std::size_t index, int line, std::string_view value, int &target)
    {
        if (scalars[index]) { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_FIELD, line, "Duplicate scalar field"); return; }
        scalars[index] = true; addInteger(value, line, target);
    };

    for (std::size_t i = 1; i < endIndex; ++i)
    {
        const int lineNumber = static_cast<int>(i + 1);
        const std::string_view line = lines[i];
        if (line.empty()) { result.AddIssue(PlayerSaveTextIssueCode::EMPTY_LINE, lineNumber, "Empty line"); continue; }
        if (line.find_first_of(" \t") != std::string_view::npos)
        { result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD, lineNumber, "Whitespace is not allowed"); continue; }
        const std::size_t equals = line.find('=');
        if (equals == std::string_view::npos || equals == 0 || equals + 1 == line.size() || line.find('=', equals + 1) != std::string_view::npos)
        { result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD, lineNumber, "Expected exactly one non-empty key=value record"); continue; }
        const auto key = line.substr(0, equals);
        const auto value = line.substr(equals + 1);
        if (key == "version") { if (!scalars[0]) { scalars[0] = true; save.version = version; } continue; }
        if (key == "position_x") { scalar(1, lineNumber, value, save.positionX); continue; }
        if (key == "position_y") { scalar(2, lineNumber, value, save.positionY); continue; }
        if (key == "current_health") { scalar(3, lineNumber, value, save.currentHealth); continue; }
        if (key == "inventory_count" || key == "skill_count" || key == "equipment_count")
        {
            const std::size_t index = key == "inventory_count" ? 4 : key == "skill_count" ? 5 : 6;
            int count = 0;
            if (scalars[index]) { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_FIELD, lineNumber, "Duplicate count"); continue; }
            scalars[index] = true;
            if (addInteger(value, lineNumber, count) && count != (index == 4 ? 28 : 5))
                result.AddIssue(PlayerSaveTextIssueCode::INVALID_COUNT, lineNumber, "Incorrect record count");
            continue;
        }
        if (key.starts_with("inventory."))
        {
            int index = -1;
            const auto indexText = key.substr(10);
            if (ParseInteger(indexText, index) != IntegerResult::VALID || index < 0 || index >= static_cast<int>(Inventory::SlotCount))
            { result.AddIssue(PlayerSaveTextIssueCode::INVALID_INVENTORY_INDEX, lineNumber, "Invalid inventory index"); continue; }
            if (inventorySeen[static_cast<std::size_t>(index)])
            { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_INVENTORY_INDEX, lineNumber, "Duplicate inventory index"); continue; }
            inventorySeen[static_cast<std::size_t>(index)] = true;
            const auto comma = value.find(',');
            if (comma == std::string_view::npos || comma == 0 || comma + 1 == value.size() || value.find(',', comma + 1) != std::string_view::npos)
            { result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD, lineNumber, "Invalid inventory value"); continue; }
            const auto item = PersistenceTokenCodec::TryParseItemToken(value.substr(0, comma));
            if (!item) { result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_ITEM_TOKEN, lineNumber, "Unknown item token"); continue; }
            int quantity = 0;
            if (addInteger(value.substr(comma + 1), lineNumber, quantity)) save.inventorySlots[static_cast<std::size_t>(index)] = {*item, quantity};
            continue;
        }
        if (key.starts_with("skill."))
        {
            const auto type = PersistenceTokenCodec::TryParseSkillToken(key.substr(6));
            if (!type) { result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_SKILL_TOKEN, lineNumber, "Unknown skill token"); continue; }
            const int index = SkillIndex(*type);
            if (skillsSeen[static_cast<std::size_t>(index)]) { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_SKILL_RECORD, lineNumber, "Duplicate skill"); continue; }
            skillsSeen[static_cast<std::size_t>(index)] = true; addInteger(value, lineNumber, skillXP[static_cast<std::size_t>(index)]); continue;
        }
        if (key.starts_with("equipment."))
        {
            const auto type = PersistenceTokenCodec::TryParseEquipmentSlotToken(key.substr(10));
            if (!type) { result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_EQUIPMENT_SLOT_TOKEN, lineNumber, "Unknown equipment slot token"); continue; }
            const int index = SlotIndex(*type);
            if (equipmentSeen[static_cast<std::size_t>(index)]) { result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_EQUIPMENT_SLOT_RECORD, lineNumber, "Duplicate equipment slot"); continue; }
            equipmentSeen[static_cast<std::size_t>(index)] = true;
            const auto item = PersistenceTokenCodec::TryParseItemToken(value);
            if (!item) { result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_ITEM_TOKEN, lineNumber, "Unknown item token"); continue; }
            equipmentItems[static_cast<std::size_t>(index)] = *item; continue;
        }
        if(key=="quest.GATHERING_BASICS")
        {
            if(version==1||questSeen){result.AddIssue(PlayerSaveTextIssueCode::DUPLICATE_FIELD,lineNumber,"Unexpected or duplicate quest record");continue;} questSeen=true; const auto comma=value.find(','); if(comma==std::string_view::npos){result.AddIssue(PlayerSaveTextIssueCode::MALFORMED_RECORD,lineNumber,"Malformed quest record");continue;} const auto state=ParseQuestState(value.substr(0,comma)); if(!state){result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_FIELD,lineNumber,"Unknown quest state");continue;} save.gatheringBasicsState=*state; addInteger(value.substr(comma+1),lineNumber,save.gatheringBasicsProgress); continue;
        }
        result.AddIssue(PlayerSaveTextIssueCode::UNKNOWN_FIELD, lineNumber, "Unknown field");
    }

    for (std::size_t i = 0; i < scalars.size(); ++i)
        if (!scalars[i]) result.AddIssue(PlayerSaveTextIssueCode::MISSING_FIELD, -1, "Missing scalar field");
    for (std::size_t i = 0; i < inventorySeen.size(); ++i)
        if (!inventorySeen[i]) result.AddIssue(PlayerSaveTextIssueCode::MISSING_INVENTORY_INDEX, -1, "Missing inventory index");
    for (std::size_t i = 0; i < skillsSeen.size(); ++i)
        if (!skillsSeen[i]) result.AddIssue(PlayerSaveTextIssueCode::MISSING_SKILL_RECORD, -1, "Missing skill record");
    for (std::size_t i = 0; i < equipmentSeen.size(); ++i)
        if (!equipmentSeen[i]) result.AddIssue(PlayerSaveTextIssueCode::MISSING_EQUIPMENT_SLOT_RECORD, -1, "Missing equipment record");
    if(version==2&&!questSeen) result.AddIssue(PlayerSaveTextIssueCode::MISSING_FIELD,-1,"Missing Gathering Basics quest record");
    if (!result.GetIssues().empty()) return result;

    save.skills.reserve(5); save.equipment.reserve(5);
    for (std::size_t i = 0; i < 5; ++i)
    {
        save.skills.push_back({PERSISTED_SKILL_TYPES[i], skillXP[i]});
        save.equipment.push_back({PERSISTED_EQUIPMENT_SLOT_TYPES[i], equipmentItems[i]});
    }
    // Version 1 had no quest section. Successful legacy decoding migrates the
    // in-memory representation to the current schema without inferring progress.
    if (save.version == 1)
    {
        save.version = CURRENT_PLAYER_SAVE_VERSION;
        save.gatheringBasicsState = QuestState::AVAILABLE;
        save.gatheringBasicsProgress = 0;
    }
    result.validationReport = PlayerSaveState::Validate(save);
    if (!result.validationReport.IsValid())
    { result.AddIssue(PlayerSaveTextIssueCode::SAVE_DATA_VALIDATION_FAILED, -1, "Decoded save data failed semantic validation"); return result; }
    result.saveData = std::move(save);
    return result;
}
