#include "StarterWorldRuntimeInput.h"

#include "../Export/StarterWorldSnapshotInput.h"
#include "../../World/Development/DevelopmentWorldContent.h"

#include <array>
#include <fstream>
#include <queue>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace StarterWorldImport
{
    namespace
    {
        [[noreturn]] void Invalid(const std::string &reason)
        {
            throw std::runtime_error("starter world runtime input: " + reason);
        }

        constexpr std::array<std::pair<int, int>, 4> Neighbors{
            {{0, -1}, {1, 0}, {0, 1}, {-1, 0}}};
    }

    StarterWorldExport::Snapshot ValidateForRuntime(
        const StarterWorldExport::Snapshot &candidate)
    {
        using namespace StarterWorldExport;
        if (candidate.width != DevelopmentWorldContent::MapWidth ||
            candidate.height != DevelopmentWorldContent::MapHeight)
            Invalid("runtime import currently requires the 100 x 100 starter map");

        // The same parser checks file input and directly supplied typed models.
        // Canonical round-trip rebinds all stable IDs, typed definitions, spawn
        // configuration and lifecycle keys before any coordinate is indexed.
        Snapshot snapshot = ParseSnapshot(SerializeCanonical(candidate));
        const int width = snapshot.width;
        const int height = snapshot.height;
        const auto index = [width](int x, int y) {
            return static_cast<std::size_t>(y) * width + x;
        };
        const auto inBounds = [width, height](int x, int y) {
            return x >= 0 && y >= 0 && x < width && y < height;
        };
        const std::size_t cells = static_cast<std::size_t>(width) * height;
        std::vector<bool> blocked(cells, false);
        std::vector<std::string> occupants(cells);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                blocked[index(x, y)] = snapshot.terrainRows[y][x] == 'W';
        for (const auto &b : snapshot.buildings)
            for (int y = b.y; y < b.y + b.height; ++y)
                for (int x = b.x; x < b.x + b.width; ++x)
                    if (b.IsWall(x, y)) blocked[index(x, y)] = true;

        for (const Placement &placement : snapshot.placements)
        {
            const int x = placement.position.GetX();
            const int y = placement.position.GetY();
            const std::size_t cell = index(x, y);
            if (!occupants[cell].empty())
                Invalid("overlapping placements: " + occupants[cell] + " and " + placement.id);
            if (blocked[cell])
                Invalid("placement requires walkable ground: " + placement.id);
            occupants[cell] = placement.id;
            if (placement.kind == PlacementKind::Resource ||
                placement.kind == PlacementKind::Station)
                blocked[cell] = true;
        }

        const int spawnX = snapshot.playerSpawn.GetX();
        const int spawnY = snapshot.playerSpawn.GetY();
        const std::size_t spawnCell = index(spawnX, spawnY);
        if (blocked[spawnCell] || !occupants[spawnCell].empty())
            Invalid("player spawn must be walkable and clear of authored placements");

        // Cardinal connectivity is equivalent to the runtime's eight-direction
        // connectivity when diagonal corner cutting is forbidden. NPCs do not
        // block tiles. Every authored service/object needs a reachable cardinal
        // approach; this also gives a legal approach under interaction rules.
        std::vector<bool> reachable(cells, false);
        std::queue<std::pair<int, int>> pending;
        reachable[spawnCell] = true;
        pending.emplace(spawnX, spawnY);
        while (!pending.empty())
        {
            const auto [x, y] = pending.front();
            pending.pop();
            for (const auto [dx, dy] : Neighbors)
            {
                const int nextX = x + dx;
                const int nextY = y + dy;
                if (!inBounds(nextX, nextY)) continue;
                const std::size_t next = index(nextX, nextY);
                if (blocked[next] || reachable[next]) continue;
                reachable[next] = true;
                pending.emplace(nextX, nextY);
            }
        }

        for (const Placement &placement : snapshot.placements)
        {
            const int x = placement.position.GetX();
            const int y = placement.position.GetY();
            const bool object = placement.kind == PlacementKind::Resource ||
                placement.kind == PlacementKind::Station;
            if (!object && !reachable[index(x, y)])
                Invalid("unreachable actor spawn: " + placement.id);
            bool hasApproach = false;
            for (const auto [dx, dy] : Neighbors)
                if (inBounds(x + dx, y + dy) && reachable[index(x + dx, y + dy)])
                    hasApproach = true;
            if (!hasApproach)
                Invalid("no reachable approach tile: " + placement.id);
        }
        for (const auto &b : snapshot.buildings)
        {
            const auto door = index(b.x + b.doorOffset, b.y + b.height - 1);
            const auto approach = index(b.x + b.doorOffset, b.y + b.height);
            if (!reachable[door] || !reachable[approach] || !occupants[door].empty() || !occupants[approach].empty())
                Invalid("building requires a clear reachable doorway and outside approach: " + b.id);
            for (int y = b.y + 1; y < b.y + b.height - 1; ++y)
                for (int x = b.x + 1; x < b.x + b.width - 1; ++x)
                    if (!blocked[index(x, y)] && !reachable[index(x, y)])
                        Invalid("unreachable building interior: " + b.id);
        }
        return snapshot;
    }

    StarterWorldExport::Snapshot LoadFile(const std::filesystem::path &path)
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(path, error) || error)
            Invalid("cannot read regular snapshot file: " + path.string());
        const std::uintmax_t size = std::filesystem::file_size(path, error);
        if (error)
            Invalid("cannot determine snapshot size: " + path.string());
        if (size > StarterWorldExport::MaximumSnapshotInputBytes)
            Invalid("snapshot file exceeds 2 MiB: " + path.string());

        std::ifstream input(path, std::ios::binary);
        if (!input)
            Invalid("cannot open snapshot file: " + path.string());
        std::string bytes;
        bytes.reserve(static_cast<std::size_t>(size));
        std::array<char, 4096> buffer{};
        while (input)
        {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::size_t count = static_cast<std::size_t>(input.gcount());
            if (bytes.size() + count > StarterWorldExport::MaximumSnapshotInputBytes)
                Invalid("snapshot file grew beyond 2 MiB: " + path.string());
            bytes.append(buffer.data(), count);
        }
        if (input.bad() || !input.eof())
            Invalid("failed while reading snapshot file: " + path.string());
        return ValidateForRuntime(StarterWorldExport::ParseSnapshot(bytes));
    }
}
