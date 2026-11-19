#include "StarterWorldHistory.h"

#include "StarterWorldEdit.h"
#include "../Import/StarterWorldRuntimeInput.h"
#include <algorithm>

#include <stdexcept>
#include <string>
#include <utility>

namespace StarterWorldHistory
{
    namespace
    {
        [[noreturn]] void Conflict()
        {
            throw std::runtime_error(
                "CONTENT_CONFLICT: Studio history does not match the current world revision");
        }

        std::string WorldCanonical(StarterWorldExport::Snapshot snapshot)
        {
            // Undo restores authored world state, while allocation reservations
            // survive undo, branching, saving and reopening. Compare the world
            // independently of that monotonically increasing bookkeeping.
            snapshot.schemaVersion = std::max(3, snapshot.schemaVersion);
            snapshot.nextPlacementId = StarterWorldExport::MaximumNextPlacementId;
            return StarterWorldExport::SerializeCanonical(snapshot);
        }
    }

    History::History(const StarterWorldExport::Snapshot &baseline)
        : expectedCanonical_(StarterWorldExport::SerializeCanonical(
            StarterWorldImport::ValidateForRuntime(baseline)))
    {
    }

    void History::CheckRevision(const StarterWorldExport::Snapshot &current) const
    {
        // Validate first. A malformed external candidate is never accepted
        // even if its serializer happens to match a historical revision.
        const auto valid = StarterWorldImport::ValidateForRuntime(current);
        if (StarterWorldExport::SerializeCanonical(valid) != expectedCanonical_)
            Conflict();
    }

    StarterWorldExport::Snapshot History::ApplyMoves(
        const StarterWorldExport::Snapshot &current,
        const std::vector<MoveCommand> &commands)
    {
        if (commands.empty() || commands.size() > MaximumMovesPerEntry)
            throw std::runtime_error("starter world history: invalid move batch size");
        CheckRevision(current);

        Entry entry;
        entry.beforeCanonical = WorldCanonical(current);
        auto candidate = StarterWorldImport::ValidateForRuntime(current);
        for (const MoveCommand &command : commands)
        {
            auto result = StarterWorldEdit::MovePlacement(
                candidate, command.placementId, command.destination);
            if (result.changed)
                entry.moves.push_back(InverseMove{result.placementId,
                    result.previousPosition, result.newPosition});
            candidate = std::move(result.candidate);
        }
        if (entry.moves.empty())
            return candidate; // A no-op must not silently erase redo history.

        if (cursor_ >= MaximumEntries)
            throw std::runtime_error("starter world history: maximum undo depth reached");

        entry.afterCanonical = WorldCanonical(candidate);
        // Prepare the entire new history before committing. Failed allocation
        // cannot discard a redo branch or change the expected revision.
        std::vector<Entry> updated(entries_.begin(), entries_.begin() + cursor_);
        updated.push_back(std::move(entry));
        std::string newExpected = StarterWorldExport::SerializeCanonical(candidate);
        entries_.swap(updated);
        expectedCanonical_.swap(newExpected);
        ++cursor_;
        return candidate;
    }

    StarterWorldExport::Snapshot History::ApplyPaint(
        const StarterWorldExport::Snapshot &current,
        const Position &tile, char ground)
    {
        CheckRevision(current);
        auto result = StarterWorldEdit::PaintTerrain(current, tile, ground);
        if (!result.changed)
            return std::move(result.candidate); // A no-op must retain redo history.

        if (cursor_ >= MaximumEntries)
            throw std::runtime_error("starter world history: maximum undo depth reached");

        Entry entry;
        entry.beforeCanonical = WorldCanonical(current);
        entry.paints.push_back(InversePaint{
            result.position, result.previousTerrain, result.newTerrain});
        entry.afterCanonical = WorldCanonical(result.candidate);

        // Allocate the new history branch before committing. A failure must
        // not change the cursor, expected revision or previous redo entries.
        std::vector<Entry> updated(entries_.begin(), entries_.begin() + cursor_);
        updated.push_back(std::move(entry));
        std::string newExpected = StarterWorldExport::SerializeCanonical(result.candidate);
        entries_.swap(updated);
        expectedCanonical_.swap(newExpected);
        ++cursor_;
        return std::move(result.candidate);
    }

    void History::Record(Entry entry, const StarterWorldExport::Snapshot &candidate)
    {
        if (cursor_ >= MaximumEntries)
            throw std::runtime_error("starter world history: maximum undo depth reached");
        entry.afterCanonical = WorldCanonical(candidate);
        std::vector<Entry> updated(entries_.begin(), entries_.begin() + cursor_);
        updated.push_back(std::move(entry));
        std::string newExpected = StarterWorldExport::SerializeCanonical(candidate);
        entries_.swap(updated);
        expectedCanonical_.swap(newExpected);
        ++cursor_;
    }

    StarterWorldExport::Snapshot History::ApplyPlace(
        const StarterWorldExport::Snapshot &current,
        std::string_view definitionId, const Position &destination)
    {
        CheckRevision(current);
        auto result = StarterWorldEdit::PlaceDefinition(current, definitionId, destination);
        Entry entry;
        entry.beforeCanonical = WorldCanonical(current);
        entry.added.push_back(result.placement);
        Record(std::move(entry), result.candidate);
        return std::move(result.candidate);
    }

    StarterWorldExport::Snapshot History::ApplyRemove(
        const StarterWorldExport::Snapshot &current, std::string_view placementId)
    {
        CheckRevision(current);
        auto result = StarterWorldEdit::RemovePlacement(current, placementId);
        Entry entry;
        entry.beforeCanonical = WorldCanonical(current);
        entry.removed.push_back(result.placement);
        Record(std::move(entry), result.candidate);
        return std::move(result.candidate);
    }

    StarterWorldExport::Snapshot History::ApplySpawnSettings(
        const StarterWorldExport::Snapshot &current,
        std::string_view placementId, const StarterWorldEdit::SpawnSettings &settings)
    {
        CheckRevision(current);
        auto result = StarterWorldEdit::ConfigureSpawn(current, placementId, settings);
        if (!result.changed)
            return std::move(result.candidate);
        Entry entry;
        entry.beforeCanonical = WorldCanonical(current);
        entry.spawns.push_back({result.placementId, result.before, result.after});
        Record(std::move(entry), result.candidate);
        return std::move(result.candidate);
    }

    StarterWorldExport::Snapshot History::Undo(
        const StarterWorldExport::Snapshot &current)
    {
        if (!CanUndo())
            throw std::runtime_error("starter world history: nothing to undo");
        CheckRevision(current);
        const Entry &entry = entries_[cursor_ - 1];
        if (entry.afterCanonical != WorldCanonical(current))
            Conflict();

        auto candidate = StarterWorldImport::ValidateForRuntime(current);
        for (auto it = entry.spawns.rbegin(); it != entry.spawns.rend(); ++it)
            candidate = StarterWorldEdit::ConfigureSpawn(
                candidate, it->placementId, it->before).candidate;
        for (const auto &placement : entry.added)
            candidate = StarterWorldEdit::RemovePlacement(candidate, placement.id).candidate;
        for (const auto &placement : entry.removed)
            candidate = StarterWorldEdit::RestorePlacement(candidate, placement);
        for (auto it = entry.paints.rbegin(); it != entry.paints.rend(); ++it)
            candidate = StarterWorldEdit::PaintTerrain(
                candidate, it->tile, it->before).candidate;
        for (auto it = entry.moves.rbegin(); it != entry.moves.rend(); ++it)
            candidate = StarterWorldEdit::MovePlacement(
                candidate, it->placementId, it->before).candidate;
        if (WorldCanonical(candidate) != entry.beforeCanonical)
            Conflict();

        std::string newExpected = StarterWorldExport::SerializeCanonical(candidate);
        expectedCanonical_.swap(newExpected);
        --cursor_;
        return candidate;
    }

    StarterWorldExport::Snapshot History::Redo(
        const StarterWorldExport::Snapshot &current)
    {
        if (!CanRedo())
            throw std::runtime_error("starter world history: nothing to redo");
        CheckRevision(current);
        const Entry &entry = entries_[cursor_];
        if (entry.beforeCanonical != WorldCanonical(current))
            Conflict();

        auto candidate = StarterWorldImport::ValidateForRuntime(current);
        for (const auto &placement : entry.removed)
            candidate = StarterWorldEdit::RemovePlacement(candidate, placement.id).candidate;
        for (const auto &placement : entry.added)
            candidate = StarterWorldEdit::RestorePlacement(candidate, placement);
        for (const InverseMove &move : entry.moves)
            candidate = StarterWorldEdit::MovePlacement(
                candidate, move.placementId, move.after).candidate;
        for (const InversePaint &paint : entry.paints)
            candidate = StarterWorldEdit::PaintTerrain(
                candidate, paint.tile, paint.after).candidate;
        for (const auto &spawn : entry.spawns)
            candidate = StarterWorldEdit::ConfigureSpawn(
                candidate, spawn.placementId, spawn.after).candidate;
        if (WorldCanonical(candidate) != entry.afterCanonical)
            Conflict();

        std::string newExpected = StarterWorldExport::SerializeCanonical(candidate);
        expectedCanonical_.swap(newExpected);
        ++cursor_;
        return candidate;
    }
}
