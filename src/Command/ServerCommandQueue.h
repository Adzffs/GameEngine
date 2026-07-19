#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

#include "ServerCommand.h"

class ServerCommandQueue
{
public:
    std::uint64_t Enqueue(ServerCommandData command);

    bool IsEmpty() const;
    std::size_t GetCount() const;
    std::optional<ServerCommand> PopNext();

private:
    std::deque<ServerCommand> commands;

    // One is the deterministic first server-assigned command ID.
    std::uint64_t nextCommandID = 1;
};
