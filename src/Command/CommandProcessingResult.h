#pragma once

#include <cstdint>

enum class CommandResultCode
{
    ACCEPTED,
    INVALID_ACTOR,
    INVALID_COMMAND_DATA,
    GAMEPLAY_REJECTED
};

struct CommandProcessingResult
{
    std::uint64_t commandID;
    int actorEntityID;
    CommandResultCode resultCode;
};
