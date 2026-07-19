#pragma once

#include "DialogueId.h"
#include "DialogueChoiceId.h"
#include "DialogueNodeId.h"
#include "DialogueNodeKind.h"

#include <optional>
#include <string>
#include <vector>

struct DialogueChoiceDefinition
{
    DialogueChoiceId id = DialogueChoiceId::NONE;
    std::string text;
    DialogueNodeId destinationNodeId = DialogueNodeId::NONE;
};

struct DialogueNodeDefinition
{
    DialogueNodeId id = DialogueNodeId::NONE;
    std::string text;
    DialogueNodeKind kind = DialogueNodeKind::TERMINAL;
    std::optional<DialogueNodeId> nextNodeId;
    std::vector<DialogueChoiceDefinition> choices;
};

struct DialogueDefinition
{
    DialogueId id = DialogueId::NONE;
    DialogueNodeId startNodeId = DialogueNodeId::NONE;
    std::vector<DialogueNodeDefinition> nodes;
};
