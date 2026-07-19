#pragma once

#include "PlayerSaveData.h"
#include "PlayerSaveValidation.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

inline constexpr std::size_t MAX_PLAYER_SAVE_TEXT_BYTES = 64 * 1024;
inline constexpr std::size_t MAX_PLAYER_SAVE_TEXT_LINES = 128;
inline constexpr std::size_t MAX_PLAYER_SAVE_TEXT_LINE_LENGTH = 256;

enum class PlayerSaveTextIssueCode
{
    EMPTY_INPUT, INPUT_TOO_LARGE, TOO_MANY_LINES, LINE_TOO_LONG, EMBEDDED_NULL,
    INVALID_LINE_ENDING, INVALID_HEADER, INVALID_END_MARKER, DATA_AFTER_END_MARKER, EMPTY_LINE,
    MALFORMED_RECORD, UNKNOWN_FIELD, DUPLICATE_FIELD, MISSING_FIELD,
    INVALID_INTEGER, NON_CANONICAL_INTEGER, INTEGER_OUT_OF_RANGE, INVALID_COUNT,
    INVALID_INVENTORY_INDEX, DUPLICATE_INVENTORY_INDEX, MISSING_INVENTORY_INDEX,
    UNKNOWN_ITEM_TOKEN, UNKNOWN_SKILL_TOKEN, DUPLICATE_SKILL_RECORD, MISSING_SKILL_RECORD,
    UNKNOWN_EQUIPMENT_SLOT_TOKEN, DUPLICATE_EQUIPMENT_SLOT_RECORD, MISSING_EQUIPMENT_SLOT_RECORD,
    UNSUPPORTED_VERSION, SAVE_DATA_VALIDATION_FAILED
};

struct PlayerSaveTextIssue
{
    PlayerSaveTextIssueCode code;
    int lineNumber = -1;
    std::string message;
};

class PlayerSaveTextDecodeResult
{
public:
    bool IsSuccess() const;
    const std::vector<PlayerSaveTextIssue> &GetIssues() const;
    bool Contains(PlayerSaveTextIssueCode code) const;
    const std::optional<PlayerSaveData> &GetSaveData() const;
    const PlayerSaveValidationReport &GetValidationReport() const;

private:
    friend class PlayerSaveTextCodec;
    void AddIssue(PlayerSaveTextIssueCode code, int lineNumber, std::string message);
    std::vector<PlayerSaveTextIssue> issues;
    std::optional<PlayerSaveData> saveData;
    PlayerSaveValidationReport validationReport;
};

class PlayerSaveTextCodec
{
public:
    static bool TryEncode(const PlayerSaveData &saveData, std::string &output,
                          PlayerSaveValidationReport &validationReport);
    static PlayerSaveTextDecodeResult Decode(std::string_view text);
};
