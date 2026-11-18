#pragma once

#include "../Export/StarterWorldSnapshot.h"

#include <filesystem>

// The first runtime bridge deliberately accepts only the existing starter
// registry and map dimensions. It changes geography, not gameplay definitions.
namespace StarterWorldImport
{
    // Checks structural bindings, complete occupancy, spawn safety and access
    // before World allocates or mutates a runtime map. Returns an owned,
    // normalized copy; never modifies the caller's candidate or a live world.
    // Throws std::runtime_error with a diagnostic on unsupported/unsafe input.
    StarterWorldExport::Snapshot ValidateForRuntime(
        const StarterWorldExport::Snapshot &snapshot);

    // Bounded, read-only file input. Missing, non-regular, oversized, malformed
    // or unsafe files fail explicitly; there is no implicit compiled fallback.
    StarterWorldExport::Snapshot LoadFile(const std::filesystem::path &path);
}
