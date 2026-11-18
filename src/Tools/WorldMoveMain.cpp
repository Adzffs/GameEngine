#include "WorldEditSession.h"
#include "../Content/Edit/StarterWorldDraftFile.h"
#include "../Content/Edit/StarterWorldEdit.h"
#include "../Content/Export/StarterWorldSnapshotInput.h"
#include "../Content/Import/StarterWorldRuntimeInput.h"

#include <charconv>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
    bool ParseCoordinate(std::string_view text, int &value)
    {
        if (text.empty()) return false;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    }

    void Usage(std::ostream &output)
    {
        output << "Usage:\n"
            "  EmbervaleWorldMove --hash <snapshot.json>\n"
            "  EmbervaleWorldMove <snapshot.json> <placement-id> <x> <y>\n"
            "  EmbervaleWorldMove --paint <snapshot.json> <x> <y> <G|D|S|W>\n"
            "  EmbervaleWorldMove --paint <snapshot.json> <x> <y> <G|D|S|W>"
            " --expected-hash <sha256> --write-draft <new-sibling.json>\n"
            "  EmbervaleWorldMove <snapshot.json> <placement-id> <x> <y>"
            " --expected-hash <sha256> --write-draft <new-sibling.json>\n"
            "  EmbervaleWorldMove --session <snapshot.json>"
            " [--expected-hash <sha256>]\n"
            "The default is a dry run. Draft writes require a fresh sibling"
            " filename and the exact input-file SHA-256. Source and player saves"
            " are never overwritten.\n";
    }

    int FailureCode(std::string_view message)
    {
        if (message.find("CONTENT_CONFLICT") != std::string_view::npos ||
            message.find("output already exists") != std::string_view::npos)
            return 4;
        if (message.find("starter world draft:") != std::string_view::npos)
            return 5;
        return 3;
    }
}

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--help")
    {
        Usage(std::cout);
        return 0;
    }
    if ((argc == 3 || argc == 5) &&
        std::string_view(argv[1]) == "--session")
    {
        if (argc == 5 && (std::string_view(argv[3]) != "--expected-hash" ||
            !StarterWorldDraftFile::ValidDigest(argv[4])))
        {
            Usage(std::cerr);
            return 2;
        }
        try
        {
            return EmbervaleStudioSession::Run(
                argv[2], std::cin, std::cout, std::cerr,
                argc == 5 ? std::string_view(argv[4]) : std::string_view{});
        }
        catch (const std::exception &error)
        {
            std::cerr << error.what() << '\n';
            return FailureCode(error.what());
        }
    }
    if (argc == 3 && std::string_view(argv[1]) == "--hash")
    {
        try
        {
            const std::string bytes = StarterWorldDraftFile::Read(argv[2]);
            (void)StarterWorldImport::ValidateForRuntime(StarterWorldExport::ParseSnapshot(bytes));
            std::cout << StarterWorldDraftFile::Digest(bytes) << '\n';
            return 0;
        }
        catch (const std::exception &error)
        {
            std::cerr << error.what() << '\n';
            return FailureCode(error.what());
        }
    }

    // Bounded single-tile terrain edits share the runtime's map, actor
    // connectivity, player-spawn and placement validation. Dry-run by
    // default; publication only through the same guarded sibling writer.
    if ((argc == 6 || argc == 10) && std::string_view(argv[1]) == "--paint")
    {
        const bool writePaint = argc == 10;
        int x = 0, y = 0;
        const std::string_view glyph(argv[5]);
        if (!ParseCoordinate(argv[3], x) || !ParseCoordinate(argv[4], y) ||
            glyph.size() != 1 ||
            (writePaint && (std::string_view(argv[6]) != "--expected-hash" ||
                !StarterWorldDraftFile::ValidDigest(argv[7]) ||
                std::string_view(argv[8]) != "--write-draft" ||
                std::string_view(argv[9]).empty())))
        {
            Usage(std::cerr);
            return 2;
        }
        try
        {
            const std::string bytes = StarterWorldDraftFile::Read(argv[2]);
            const std::string sourceHash = StarterWorldDraftFile::Digest(bytes);
            if (writePaint && sourceHash != argv[7])
            {
                std::cerr << "CONTENT_CONFLICT: source SHA-256 differs from expected-hash\n";
                return 4;
            }
            const auto source = StarterWorldImport::ValidateForRuntime(
                StarterWorldExport::ParseSnapshot(bytes));
            const auto result = StarterWorldEdit::PaintTerrain(
                source, Position{x, y}, glyph[0]);
            if (writePaint)
            {
                const std::string content =
                    StarterWorldExport::SerializeCanonical(result.candidate);
                StarterWorldDraftFile::PublishNew(argv[2], argv[7], content, argv[9]);
                std::cout << "Saved new draft: " << argv[9] << "\n"
                    << "source-sha256: " << sourceHash << "\n"
                    << "draft-sha256: " << StarterWorldDraftFile::Digest(content) << '\n';
            }
            else
            {
                std::cout << (result.changed ? "Valid paint: " : "Valid unchanged terrain: ")
                    << "(" << x << ", " << y << ") "
                    << result.previousTerrain << " -> " << result.newTerrain
                    << "; dry run, no files written.\n"
                    << "source-sha256: " << sourceHash << '\n';
            }
            return 0;
        }
        catch (const std::exception &error)
        {
            std::cerr << error.what() << '\n';
            return FailureCode(error.what());
        }
    }

    const bool write = argc == 9;
    int x = 0;
    int y = 0;
    if ((argc != 5 && !write) || std::string_view(argv[1]).empty() ||
        std::string_view(argv[1]).starts_with("--") ||
        std::string_view(argv[2]).empty() ||
        !ParseCoordinate(argv[3], x) || !ParseCoordinate(argv[4], y) ||
        (write && (std::string_view(argv[5]) != "--expected-hash" ||
            !StarterWorldDraftFile::ValidDigest(argv[6]) ||
            std::string_view(argv[7]) != "--write-draft" ||
            std::string_view(argv[8]).empty())))
    {
        Usage(std::cerr);
        return 2;
    }

    try
    {
        // Parse and validate the exact bytes whose fingerprint is displayed.
        // Do not allow source changes after this read to change the candidate
        // unnoticed: PublishNew rereads/rechecks under the cooperative lock.
        const std::string sourceBytes = StarterWorldDraftFile::Read(argv[1]);
        const std::string sourceHash = StarterWorldDraftFile::Digest(sourceBytes);
        if (write && sourceHash != argv[6])
        {
            std::cerr << "CONTENT_CONFLICT: source SHA-256 differs from expected-hash\n";
            return 4;
        }
        const auto source = StarterWorldImport::ValidateForRuntime(
            StarterWorldExport::ParseSnapshot(sourceBytes));
        const auto result = StarterWorldEdit::MovePlacement(source, argv[2], Position{x, y});
        if (write)
        {
            const std::string candidate = StarterWorldExport::SerializeCanonical(result.candidate);
            StarterWorldDraftFile::PublishNew(argv[1], argv[6], candidate, argv[8]);
            std::cout << "Saved new draft: " << argv[8] << "\n"
                << "source-sha256: " << sourceHash << "\n"
                << "draft-sha256: " << StarterWorldDraftFile::Digest(candidate) << '\n';
        }
        else
        {
            std::cout << (result.changed ? "Valid move: " : "Valid unchanged placement: ")
                << result.placementId << " (" << result.previousPosition.GetX() << ", "
                << result.previousPosition.GetY() << ") -> ("
                << result.newPosition.GetX() << ", " << result.newPosition.GetY()
                << "); dry run, no files written.\n"
                << "source-sha256: " << sourceHash << '\n';
        }
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return FailureCode(error.what());
    }
}
