#include "WorldEditSession.h"

#include "../Content/Edit/StarterWorldDraftFile.h"
#include "../Content/Edit/StarterWorldHistory.h"
#include "../Content/Export/StarterWorldSnapshotInput.h"
#include "../Content/Import/StarterWorldRuntimeInput.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
    bool Coordinate(std::string_view text, int &value)
    {
        if (text.empty()) return false;
        const auto [end, error] = std::from_chars(
            text.data(), text.data() + text.size(), value);
        return error == std::errc{} && end == text.data() + text.size();
    }

    void RequireEnd(std::istringstream &args)
    {
        std::string extra;
        if (args >> extra)
            throw std::runtime_error("unexpected extra command arguments");
    }

    char TerrainGlyph(char symbol)
    {
        switch (symbol)
        {
        case 'G': return '.';
        case 'D': return ':';
        case 'S': return '#';
        case 'W': return '~';
        }
        return '?';
    }

    char PlacementGlyph(StarterWorldExport::PlacementKind kind)
    {
        using Kind = StarterWorldExport::PlacementKind;
        switch (kind)
        {
        case Kind::Resource: return 'R';
        case Kind::Station: return 'S';
        case Kind::NpcSpawn: return 'N';
        case Kind::MonsterSpawn: return 'M';
        }
        return '?';
    }

    void PrintMap(const StarterWorldExport::Snapshot &snapshot,
        int centerX, int centerY, std::ostream &out)
    {
        if (centerX < 0 || centerY < 0 ||
            centerX >= snapshot.width || centerY >= snapshot.height)
            throw std::runtime_error("map center is outside the starter region");

        constexpr int Radius = 10;
        const int left = std::max(0, centerX - Radius);
        const int right = std::min(snapshot.width - 1, centerX + Radius);
        const int top = std::max(0, centerY - Radius);
        const int bottom = std::min(snapshot.height - 1, centerY + Radius);
        out << "Map x=" << left << ".." << right << " y="
            << top << ".." << bottom << " (each glyph is one tile)\n"
            << "Legend . grass, : dirt, # stone, ~ water,"
            << " R resource, S station, N npc, M monster, @ spawn\n";
        for (int y = top; y <= bottom; ++y)
        {
            std::string row;
            row.reserve(static_cast<std::size_t>(right - left + 1));
            for (int x = left; x <= right; ++x)
                row.push_back(TerrainGlyph(snapshot.terrainRows[y][x]));
            for (const auto &placement : snapshot.placements)
            {
                const int x = placement.position.GetX();
                if (placement.position.GetY() == y && x >= left && x <= right)
                    row[static_cast<std::size_t>(x - left)] =
                        PlacementGlyph(placement.kind);
            }
            if (snapshot.playerSpawn.GetY() == y &&
                snapshot.playerSpawn.GetX() >= left &&
                snapshot.playerSpawn.GetX() <= right)
                row[static_cast<std::size_t>(snapshot.playerSpawn.GetX() - left)] = '@';
            out << std::setw(3) << y << "  " << row << '\n';
        }
    }

    void PrintPlacements(const StarterWorldExport::Snapshot &snapshot,
        std::ostream &out)
    {
        using Kind = StarterWorldExport::PlacementKind;
        out << "Existing placements (only R and S can be moved):\n";
        for (const auto &placement : snapshot.placements)
        {
            out << PlacementGlyph(placement.kind) << "  " << placement.id
                << "  (" << placement.position.GetX() << ", "
                << placement.position.GetY() << ")";
            if (placement.kind != Kind::Resource && placement.kind != Kind::Station)
                out << "  [read-only spawn]";
            out << '\n';
        }
    }

    void Help(std::ostream &out)
    {
        out << "Commands: help, list, map [x y], move <id> <x> <y>,"
            << " undo, redo, status, save <fresh-sibling.json>, quit\n"
            << "All changes are private until save. A save creates a NEW sibling"
            << " draft only; it cannot overwrite source, existing drafts or"
            << " player saves. No actions affect the running game.\n";
    }
}

namespace EmbervaleStudioSession
{
    int Run(const std::filesystem::path &sourcePath,
        std::istream &input, std::ostream &output, std::ostream &errors)
    {
        const std::string bytes = StarterWorldDraftFile::Read(sourcePath);
        const std::string expectedHash = StarterWorldDraftFile::Digest(bytes);
        auto current = StarterWorldImport::ValidateForRuntime(
            StarterWorldExport::ParseSnapshot(bytes));
        const std::string baselineCanonical =
            StarterWorldExport::SerializeCanonical(current);
        StarterWorldHistory::History history(current);

        output << "Embervale Studio — starter-world editing session\n"
            << "Source SHA-256: " << expectedHash << '\n';
        Help(output);

        std::string line;
        while (std::getline(input, line))
        {
            // Avoid letting an accidentally pasted large document become a command.
            if (line.size() > 4096)
            {
                errors << "Error: command exceeds 4096 characters\n";
                continue;
            }
            std::istringstream args(line);
            std::string command;
            if (!(args >> command)) continue;
            try
            {
                if (command == "help")
                {
                    RequireEnd(args);
                    Help(output);
                }
                else if (command == "list")
                {
                    RequireEnd(args);
                    PrintPlacements(current, output);
                }
                else if (command == "map")
                {
                    int x = current.playerSpawn.GetX();
                    int y = current.playerSpawn.GetY();
                    std::string sx;
                    if (args >> sx)
                    {
                        std::string sy;
                        if (!(args >> sy) || !Coordinate(sx, x) ||
                            !Coordinate(sy, y))
                            throw std::runtime_error("usage: map [x y]");
                    }
                    RequireEnd(args);
                    PrintMap(current, x, y, output);
                }
                else if (command == "move")
                {
                    std::string id, sx, sy;
                    int x = 0, y = 0;
                    if (!(args >> id >> sx >> sy) ||
                        !Coordinate(sx, x) || !Coordinate(sy, y))
                        throw std::runtime_error("usage: move <id> <x> <y>");
                    RequireEnd(args);
                    auto next = history.ApplyMoves(current,
                        {{id, Position{x, y}}});
                    const bool changed =
                        StarterWorldExport::SerializeCanonical(next) !=
                            StarterWorldExport::SerializeCanonical(current);
                    current = std::move(next);
                    output << (changed ? "Moved " : "Unchanged ") << id
                        << " to (" << x << ", " << y << ")\n";
                }
                else if (command == "undo")
                {
                    RequireEnd(args);
                    current = history.Undo(current);
                    output << "Undone.\n";
                }
                else if (command == "redo")
                {
                    RequireEnd(args);
                    current = history.Redo(current);
                    output << "Redone.\n";
                }
                else if (command == "status")
                {
                    RequireEnd(args);
                    output << "Changed: " <<
                        (StarterWorldExport::SerializeCanonical(current) !=
                            baselineCanonical ? "yes" : "no")
                        << "; undo: " << history.AppliedCount()
                        << "; redo: " <<
                        (history.EntryCount() - history.AppliedCount())
                        << "; original SHA-256: " << expectedHash << '\n';
                }
                else if (command == "save")
                {
                    std::string destination;
                    if (!(args >> destination))
                        throw std::runtime_error(
                            "usage: save <fresh-sibling.json>");
                    RequireEnd(args);
                    // Revalidate against actual runtime rules before any file
                    // publication. PublishNew rereads exact input under its
                    // cooperative lock and refuses stale or occupied targets.
                    const auto validated =
                        StarterWorldImport::ValidateForRuntime(current);
                    const std::string candidate =
                        StarterWorldExport::SerializeCanonical(validated);
                    StarterWorldDraftFile::PublishNew(
                        sourcePath, expectedHash, candidate, destination);
                    output << "Saved new draft: " << destination
                        << "\ndraft-sha256: " <<
                        StarterWorldDraftFile::Digest(candidate) << '\n';
                }
                else if (command == "quit" || command == "exit")
                {
                    RequireEnd(args);
                    output << "Session closed. Unsaved edits discarded.\n";
                    break;
                }
                else
                    throw std::runtime_error("unknown command: " + command);
            }
            catch (const std::exception &error)
            {
                errors << "Error: " << error.what() << '\n';
            }
        }
        return 0;
    }
}
