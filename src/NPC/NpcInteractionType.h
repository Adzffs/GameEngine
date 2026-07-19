#pragma once

enum class NpcInteractionType
{
    TALK
};

constexpr bool IsValidNpcInteractionType(NpcInteractionType type)
{
    return type == NpcInteractionType::TALK;
}
