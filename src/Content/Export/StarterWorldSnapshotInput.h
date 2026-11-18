#pragma once

#include "StarterWorldSnapshot.h"

#include <string_view>

namespace StarterWorldExport
{
    // Parse an external snapshot without modifying the runtime or any save.
    // Accepts the current schema only; rejects unknown fields and identities.
    // Throws std::runtime_error with a diagnostic on malformed input.
    Snapshot ParseSnapshot(std::string_view input);
}
