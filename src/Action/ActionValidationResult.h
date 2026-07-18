#pragma once

#include <string>

#include "ActionCancelReason.h"

struct ActionValidationResult
{
    bool valid;
    ActionCancelReason reason;
    std::string message;
};
