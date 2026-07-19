#pragma once

enum class NpcInteractionType
{
    TALK,
    TRADE
};

constexpr bool IsValidNpcInteractionType(NpcInteractionType type)
{
    return type == NpcInteractionType::TALK || type == NpcInteractionType::TRADE;
}
