#include "ServerCommandQueue.h"

#include <utility>

std::uint64_t ServerCommandQueue::Enqueue(
    ServerCommandData command)
{
    const std::uint64_t commandID = nextCommandID++;

    commands.push_back(
        ServerCommand{
            commandID,
            std::move(command)});

    return commandID;
}

bool ServerCommandQueue::IsEmpty() const
{
    return commands.empty();
}

std::size_t ServerCommandQueue::GetCount() const
{
    return commands.size();
}

std::optional<ServerCommand>
ServerCommandQueue::PopNext()
{
    if (commands.empty())
    {
        return std::nullopt;
    }

    ServerCommand command =
        std::move(commands.front());

    commands.pop_front();
    return command;
}
