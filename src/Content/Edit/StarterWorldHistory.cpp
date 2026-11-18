#include "StarterWorldHistory.h"

#include "StarterWorldEdit.h"
#include "../Import/StarterWorldRuntimeInput.h"

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
        entry.beforeCanonical = expectedCanonical_;
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

        entry.afterCanonical = StarterWorldExport::SerializeCanonical(candidate);
        // Prepare the entire new history before committing. Failed allocation
        // cannot discard a redo branch or change the expected revision.
        std::vector<Entry> updated(entries_.begin(), entries_.begin() + cursor_);
        updated.push_back(std::move(entry));
        std::string newExpected = updated.back().afterCanonical;
        entries_.swap(updated);
        expectedCanonical_.swap(newExpected);
        ++cursor_;
        return candidate;
    }

    StarterWorldExport::Snapshot History::Undo(
        const StarterWorldExport::Snapshot &current)
    {
        if (!CanUndo())
            throw std::runtime_error("starter world history: nothing to undo");
        CheckRevision(current);
        const Entry &entry = entries_[cursor_ - 1];
        if (entry.afterCanonical != expectedCanonical_)
            Conflict();

        auto candidate = StarterWorldImport::ValidateForRuntime(current);
        for (auto it = entry.moves.rbegin(); it != entry.moves.rend(); ++it)
            candidate = StarterWorldEdit::MovePlacement(
                candidate, it->placementId, it->before).candidate;
        if (StarterWorldExport::SerializeCanonical(candidate) != entry.beforeCanonical)
            Conflict();

        std::string newExpected = entry.beforeCanonical;
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
        if (entry.beforeCanonical != expectedCanonical_)
            Conflict();

        auto candidate = StarterWorldImport::ValidateForRuntime(current);
        for (const InverseMove &move : entry.moves)
            candidate = StarterWorldEdit::MovePlacement(
                candidate, move.placementId, move.after).candidate;
        if (StarterWorldExport::SerializeCanonical(candidate) != entry.afterCanonical)
            Conflict();

        std::string newExpected = entry.afterCanonical;
        expectedCanonical_.swap(newExpected);
        ++cursor_;
        return candidate;
    }
}
