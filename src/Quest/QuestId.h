#pragma once
enum class QuestId : int
{
    NONE = 0,
    GATHERING_BASICS = 1,
    MINING_BASICS = 2,
    TRACKS_IN_THE_ASH = 3,
    SHIELD_OR_SHELTER = 4,
    FANGS_AT_THE_GATE = 5
};
constexpr bool IsValidQuestId(QuestId id)
{
    return id == QuestId::GATHERING_BASICS ||
           id == QuestId::MINING_BASICS ||
           id == QuestId::TRACKS_IN_THE_ASH ||
           id == QuestId::SHIELD_OR_SHELTER ||
           id == QuestId::FANGS_AT_THE_GATE;
}
