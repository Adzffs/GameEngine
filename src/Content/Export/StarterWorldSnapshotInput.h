#pragma once

#include "StarterWorldSnapshot.h"

#include <cstddef>
#include <string_view>

namespace StarterWorldExport
{
    inline constexpr std::size_t MaximumSnapshotInputBytes = 2 * 1024 * 1024;

    // Parse an external snapshot without modifying the runtime or any save.
    // Accepts the current ASCII schema only; rejects unknown fields and identities.
    // The byte bound is shared with file readers; decoded strings, nesting,
    // container sizes and total JSON values are bounded independently.
    // Throws std::runtime_error with a diagnostic on malformed input.
    Snapshot ParseSnapshot(std::string_view input);
}
