#pragma once

#include "DialogueChoiceId.h"

#include <string>

struct DialogueEventChoice
{
    DialogueChoiceId id = DialogueChoiceId::NONE;
    std::string text;
};
