#pragma once

#include "DialogueId.h"
#include "DialogueNodeId.h"

#include <optional>
#include <string>
#include <vector>

struct DialogueNodeDefinition
{
    DialogueNodeId id = DialogueNodeId::NONE;
    std::string text;
    std::optional<DialogueNodeId> nextNodeId;
};

struct DialogueDefinition
{
    DialogueId id = DialogueId::NONE;
    DialogueNodeId startNodeId = DialogueNodeId::NONE;
    std::vector<DialogueNodeDefinition> nodes;
};
