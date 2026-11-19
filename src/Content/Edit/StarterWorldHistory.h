#pragma once

#include "../Export/StarterWorldSnapshot.h"

#include <cstddef>
#include <string>
#include <vector>

// In-memory Studio command history. It never touches a live World, draft, or
// player save. Each inverse is an ordinary runtime-validated object/terrain edit;
// no old whole-world snapshot is blindly restored over external changes.
namespace StarterWorldHistory
{
    struct MoveCommand
    {
        std::string placementId;
        Position destination;
    };

    class History
    {
    public:
        static constexpr std::size_t MaximumEntries = 64;
        static constexpr std::size_t MaximumMovesPerEntry = 32;

        explicit History(const StarterWorldExport::Snapshot &baseline);

        // All moves are applied to a private candidate, in order. Each
        // intermediate world must remain valid. Successful changes form ONE
        // undoable entry; an all-no-op batch does not change history.
        // Throws on invalid moves or an unexpected external semantic revision.
        StarterWorldExport::Snapshot ApplyMoves(
            const StarterWorldExport::Snapshot &current,
            const std::vector<MoveCommand> &commands);

        // One accepted, validated base-ground tile change becomes one history
        // entry. No-op paint keeps any pending redo branch intact.
        StarterWorldExport::Snapshot ApplyPaint(
            const StarterWorldExport::Snapshot &current,
            const Position &tile, char ground);

        StarterWorldExport::Snapshot ApplyPlace(
            const StarterWorldExport::Snapshot &current,
            std::string_view definitionId, const Position &destination);
        StarterWorldExport::Snapshot ApplyRemove(
            const StarterWorldExport::Snapshot &current,
            std::string_view placementId);

        // Both require the exact current semantic revision previously returned
        // by this history. On conflict or invalid inverse, state is unchanged.
        StarterWorldExport::Snapshot Undo(const StarterWorldExport::Snapshot &current);
        StarterWorldExport::Snapshot Redo(const StarterWorldExport::Snapshot &current);

        bool CanUndo() const noexcept { return cursor_ != 0; }
        bool CanRedo() const noexcept { return cursor_ != entries_.size(); }
        std::size_t EntryCount() const noexcept { return entries_.size(); }
        std::size_t AppliedCount() const noexcept { return cursor_; }

    private:
        struct InverseMove
        {
            std::string placementId;
            Position before;
            Position after;
        };
        struct InversePaint
        {
            Position tile;
            char before;
            char after;
        };
        struct Entry
        {
            std::vector<InverseMove> moves;
            std::vector<InversePaint> paints;
            std::vector<StarterWorldExport::Placement> added;
            std::vector<StarterWorldExport::Placement> removed;
            std::string beforeCanonical;
            std::string afterCanonical;
        };

        void CheckRevision(const StarterWorldExport::Snapshot &current) const;
        void Record(Entry entry, const StarterWorldExport::Snapshot &candidate);
        std::vector<Entry> entries_;
        std::size_t cursor_ = 0;
        std::string expectedCanonical_;
    };
}
