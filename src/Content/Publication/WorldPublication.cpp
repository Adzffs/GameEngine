#include "WorldPublication.h"
#include "../Edit/StarterWorldDraftFile.h"
#include "../Export/StarterWorldSnapshotInput.h"
#include "../Import/StarterWorldRuntimeInput.h"
#include "../Json/BoundedJson.h"
#include "Generated/WorldValidatorRevision.h"

#include <array>
#include <stdexcept>
#include <system_error>

#ifdef GAMEENGINE_PUBLICATION_CRASH_TEST
void GameEnginePublicationTestPause(std::string_view phase);
#endif

namespace WorldPublication
{
    namespace
    {
        using Json = BoundedJson::Value;
        [[noreturn]] void Fail(const std::string &message)
        { throw std::runtime_error("world publication: " + message); }

        const Json &Field(const Json &object, const char *name)
        {
            if (object.type != Json::Type::Object || !object.object.contains(name))
                Fail(std::string("missing metadata field: ") + name);
            return object.object.at(name);
        }
        std::string Text(const Json &object, const char *name)
        {
            const auto &v = Field(object, name);
            if (v.type != Json::Type::String) Fail(std::string("expected string: ") + name);
            return v.string;
        }
        int Number(const Json &object, const char *name)
        {
            const auto &v = Field(object, name);
            if (v.type != Json::Type::Number) Fail(std::string("expected integer: ") + name);
            return v.number;
        }
        void Keys(const Json &object, std::initializer_list<const char *> names)
        {
            if (object.type != Json::Type::Object || object.object.size() != names.size())
                Fail("unexpected metadata fields");
            for (const auto *name : names) (void)Field(object, name);
        }
        void Check(const ValidationReceipt &r)
        {
            if (!StarterWorldDraftFile::ValidDigest(r.sourceSha256) ||
                !StarterWorldDraftFile::ValidDigest(r.worldSha256) ||
                !StarterWorldDraftFile::ValidDigest(r.validatorRevision))
                Fail("metadata SHA-256 must contain 64 lowercase hex characters");
            if (r.worldSchemaVersion < 1 || r.worldSchemaVersion > 4)
                Fail("unsupported world schema version");
            if (r.validatorProfile != ValidatorProfile) Fail("unsupported validator profile");
        }
        ValidationReceipt ReceiptFields(const Json &object)
        {
            ValidationReceipt r{Text(object, "source_sha256"), Text(object, "world_sha256"),
                Number(object, "world_schema_version"), Text(object, "validator_profile"),
                Text(object, "validator_revision")};
            Check(r);
            return r;
        }
        std::string Fields(const ValidationReceipt &r)
        {
            Check(r);
            return "  \"source_sha256\": \"" + r.sourceSha256 + "\",\n" +
                "  \"world_sha256\": \"" + r.worldSha256 + "\",\n" +
                "  \"world_schema_version\": " + std::to_string(r.worldSchemaVersion) + ",\n" +
                "  \"validator_profile\": \"" + r.validatorProfile + "\",\n" +
                "  \"validator_revision\": \"" + r.validatorRevision + "\"\n";
        }
    }

    std::string_view ValidatorRevision() { return EMBERVALE_WORLD_VALIDATOR_REVISION; }

    bool ValidRevision(std::string_view revision)
    {
        if (revision.empty() || revision.size() > 48) return false;
        for (const auto reserved : {"con", "prn", "aux", "nul"})
            if (revision == reserved) return false;
        if (revision.size() == 4 && (revision.starts_with("com") || revision.starts_with("lpt")) &&
            revision.back() >= '1' && revision.back() <= '9') return false;
        for (std::size_t i = 0; i < revision.size(); ++i)
        {
            const char c = revision[i];
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                  (i > 0 && c == '-'))) return false;
        }
        return revision.back() != '-';
    }

    ValidationReceipt ValidateDraft(const std::filesystem::path &source)
    {
        const auto bytes = StarterWorldDraftFile::Read(source);
        const auto snapshot = StarterWorldImport::ValidateForRuntime(StarterWorldExport::ParseSnapshot(bytes));
        const auto canonical = StarterWorldExport::SerializeCanonical(snapshot);
        return {StarterWorldDraftFile::Digest(bytes), StarterWorldDraftFile::Digest(canonical),
            snapshot.schemaVersion, std::string(ValidatorProfile), std::string(ValidatorRevision())};
    }

    std::string SerializeReceipt(const ValidationReceipt &receipt)
    { return "{\n  \"receipt_version\": 1,\n" + Fields(receipt) + "}\n"; }

    ValidationReceipt ParseReceipt(std::string_view bytes)
    {
        const auto object = BoundedJson::Parse(bytes, MaximumMetadataBytes, "world validation receipt");
        Keys(object, {"receipt_version", "source_sha256", "world_sha256", "world_schema_version",
            "validator_profile", "validator_revision"});
        if (Number(object, "receipt_version") != 1) Fail("unsupported validation receipt version");
        return ReceiptFields(object);
    }

    std::string SerializeManifest(const Manifest &manifest)
    {
        if (!ValidRevision(manifest.revision)) Fail("invalid revision name");
        return "{\n  \"manifest_version\": 1,\n  \"revision\": \"" + manifest.revision +
            "\",\n  \"world_file\": \"world.json\",\n" + Fields(manifest.validation) + "}\n";
    }

    Manifest ParseManifest(std::string_view bytes)
    {
        const auto object = BoundedJson::Parse(bytes, MaximumMetadataBytes, "world version manifest");
        Keys(object, {"manifest_version", "revision", "world_file", "source_sha256", "world_sha256",
            "world_schema_version", "validator_profile", "validator_revision"});
        if (Number(object, "manifest_version") != 1) Fail("unsupported manifest version");
        if (Text(object, "world_file") != "world.json") Fail("world_file must be the fixed world.json leaf");
        Manifest manifest{Text(object, "revision"), ReceiptFields(object)};
        if (!ValidRevision(manifest.revision)) Fail("invalid revision name");
        return manifest;
    }

    Manifest Publish(const std::filesystem::path &source, const ValidationReceipt &receipt,
        const std::filesystem::path &library, std::string_view revision)
    {
        if (!ValidRevision(revision)) Fail("use a 1-48 character lowercase name with letters, digits and hyphens");
        Check(receipt);
        if (receipt.validatorRevision != ValidatorRevision())
            Fail("validation tool changed; validate the draft again");
        if (ValidateDraft(source) != receipt) Fail("draft changed; validate it again before publishing");
        const auto sourceBytes = StarterWorldDraftFile::Read(source);
        if (StarterWorldDraftFile::Digest(sourceBytes) != receipt.sourceSha256)
            Fail("draft changed while preparing publication");
        const auto snapshot = StarterWorldImport::ValidateForRuntime(StarterWorldExport::ParseSnapshot(sourceBytes));
        const auto world = StarterWorldExport::SerializeCanonical(snapshot);
        if (StarterWorldDraftFile::Digest(world) != receipt.worldSha256) Fail("validated world changed");
        if (library.empty()) Fail("select an explicit version library directory");
        const Manifest manifest{std::string(revision), receipt};

        StarterWorldDraftFile::WithVerifiedSource(source, receipt.sourceSha256, [&] {
            if (std::filesystem::is_symlink(std::filesystem::symlink_status(library)))
                Fail("version library must not be a symbolic link");
            std::filesystem::create_directories(library);
            if (!std::filesystem::is_directory(library)) Fail("version library is not a directory");
            const auto destination = library / manifest.revision;
            if (!std::filesystem::create_directory(destination))
                Fail("version name is already reserved or published; choose a fresh name");
            bool wroteWorld = false, committed = false;
            try
            {
#ifdef GAMEENGINE_PUBLICATION_CRASH_TEST
                GameEnginePublicationTestPause("reserved");
#endif
                StarterWorldDraftFile::PublishNewBytes(world, destination / "world.json");
                wroteWorld = true;
#ifdef GAMEENGINE_PUBLICATION_CRASH_TEST
                GameEnginePublicationTestPause("world-written");
#endif
                // The manifest is the commit record. A directory without it is
                // never a published version, regardless of leftover files.
                StarterWorldDraftFile::PublishNewBytes(SerializeManifest(manifest), destination / "manifest.json");
                committed = true;
#ifdef GAMEENGINE_PUBLICATION_CRASH_TEST
                GameEnginePublicationTestPause("committed");
#endif
            }
            catch (...)
            {
                if (!committed)
                {
                    std::error_code ignored;
                    if (wroteWorld) std::filesystem::remove(destination / "world.json", ignored);
                    // Never recursively delete an incomplete directory that
                    // could now contain someone else's inspection/recovery data.
                    std::filesystem::remove(destination, ignored);
                }
                throw;
            }
        });
        return manifest;
    }

    LoadedVersion Load(const std::filesystem::path &manifestPath)
    {
        if (manifestPath.filename() != "manifest.json") Fail("select a version's manifest.json");
        const auto parent = manifestPath.parent_path().empty() ? std::filesystem::path(".") : manifestPath.parent_path();
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(parent)))
            Fail("version directory must not be a symbolic link");
        const auto bytes = StarterWorldDraftFile::Read(manifestPath, MaximumMetadataBytes);
        const auto manifest = ParseManifest(bytes);
        if (parent.filename() != manifest.revision) Fail("version directory does not match manifest revision");
        const auto world = StarterWorldDraftFile::Read(parent / "world.json");
        if (StarterWorldDraftFile::Digest(world) != manifest.validation.worldSha256)
            Fail("published world SHA-256 mismatch");
        auto snapshot = StarterWorldImport::ValidateForRuntime(StarterWorldExport::ParseSnapshot(world));
        if (snapshot.schemaVersion != manifest.validation.worldSchemaVersion)
            Fail("published world schema differs from manifest");
        if (StarterWorldExport::SerializeCanonical(snapshot) != world)
            Fail("published world must contain canonical bytes");
        if (StarterWorldDraftFile::Read(manifestPath, MaximumMetadataBytes) != bytes)
            Fail("manifest changed during loading");
        return {manifest, std::move(snapshot)};
    }
}
