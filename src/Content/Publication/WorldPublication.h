#pragma once
#include "../Export/StarterWorldSnapshot.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace WorldPublication
{
    inline constexpr std::size_t MaximumMetadataBytes = 4096;
    inline constexpr std::string_view ValidatorProfile = "starter-runtime-v1";
    std::string_view ValidatorRevision();

    struct ValidationReceipt
    {
        std::string sourceSha256;
        std::string worldSha256;
        int worldSchemaVersion = 1;
        std::string validatorProfile;
        std::string validatorRevision;
        bool operator==(const ValidationReceipt &) const = default;
    };
    struct Manifest
    {
        std::string revision;
        ValidationReceipt validation;
        bool operator==(const Manifest &) const = default;
    };
    struct LoadedVersion
    {
        Manifest manifest;
        StarterWorldExport::Snapshot snapshot;
    };

    bool ValidRevision(std::string_view revision);
    ValidationReceipt ValidateDraft(const std::filesystem::path &source);
    std::string SerializeReceipt(const ValidationReceipt &receipt);
    ValidationReceipt ParseReceipt(std::string_view bytes);
    std::string SerializeManifest(const Manifest &manifest);
    Manifest ParseManifest(std::string_view bytes);

    // The selected receipt must bind the exact source/current validator. The
    // new directory is exclusively reserved, and manifest.json is committed
    // last. Incomplete directories are never playable or silently taken over.
    Manifest Publish(const std::filesystem::path &source, const ValidationReceipt &receipt,
        const std::filesystem::path &library, std::string_view revision);

    // Accept only the fixed <library>/<revision>/manifest.json layout. Check
    // hashes/schema/profile, then revalidate with this runtime before returning
    // an owned snapshot. Older validator revisions require that same recheck.
    LoadedVersion Load(const std::filesystem::path &manifestPath);
}
