#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

// Lower-level, create-only publication for a *previously validated* canonical
// world snapshot. This deliberately cannot update a source or an existing draft.
namespace StarterWorldDraftFile
{
    constexpr std::size_t MaximumBytes = 2 * 1024 * 1024;

    // Lowercase SHA-256 of the original file bytes, not normalized JSON.
    std::string Digest(std::string_view bytes);

    // Bounded binary read; reject symlinks, directories and input over 2 MiB.
    std::string Read(const std::filesystem::path &path);

    // A valid expected hash is exactly 64 lowercase hex ASCII characters.
    bool ValidDigest(std::string_view digest);

    // Requires: candidateBytes is the canonical, runtime-validated snapshot
    // produced by the typed editor. Recheck the source *under a cooperative
    // directory lock*, then publish atomically to a fresh output file using
    // a same-filesystem hard link. Existing output is never replaced.
    // Concurrent noncooperating edits to source cannot be made fully atomic.
    // Failure leaves source/destination intact; power-loss durability is not
    // guaranteed. Throws runtime_error; does not remove another writer's lock.
    void PublishNew(const std::filesystem::path &source,
        std::string_view expectedHash, std::string_view candidateBytes,
        const std::filesystem::path &destination);
}
