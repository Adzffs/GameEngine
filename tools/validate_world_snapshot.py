#!/usr/bin/env python3
"""Read and validate Embervale's exported starter-world snapshot (not a runtime loader).

This tool deliberately does not modify game content or player saves.
"""
import argparse
import json
import re
import sys
from pathlib import Path

IDENTITY = re.compile(r"[a-z0-9._-]{1,128}\Z")
KINDS = {"resource", "station", "npc_spawn", "monster_spawn"}
STAGES = {"world_start", "first_player_registration"}
TERRAIN = {"G", "D", "S", "W"}
MAX_BYTES = 8 * 1024 * 1024


class SnapshotError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise SnapshotError(message)


def exact_keys(obj, required, optional=()):
    require(type(obj) is dict, "expected JSON object")
    keys = set(obj)
    require(set(required) <= keys and keys <= set(required) | set(optional),
            "unexpected or missing fields: " + repr(sorted(keys)))


def integer(value, minimum, maximum, field):
    require(type(value) is int and minimum <= value <= maximum,
            f"{field} must be an integer in {minimum}..{maximum}")
    return value


def identity(value, field):
    require(type(value) is str and IDENTITY.fullmatch(value) is not None,
            f"{field} is not a valid stable identifier")


def position(value, width, height):
    exact_keys(value, {"x", "y", "surface_id"})
    integer(value["x"], 0, width - 1, "position.x")
    integer(value["y"], 0, height - 1, "position.y")
    require(value["surface_id"] == "ground", "unsupported surface")


def unique_object_pairs(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise SnapshotError(f"duplicate JSON field: {key}")
        obj[key] = value
    return obj


def validate(data):
    exact_keys(data, {"format", "schema_version", "region_id", "revision",
                      "size", "surfaces", "terrain", "player_spawn",
                      "placements", "region_exits"})
    require(data["format"] == "embervale.starter-world-snapshot",
            "unsupported snapshot format")
    require(type(data["schema_version"]) is int and data["schema_version"] == 1,
            "unsupported schema version")
    identity(data["region_id"], "region_id")
    integer(data["revision"], 1, 2147483647, "revision")
    exact_keys(data["size"], {"width", "height"})
    width = integer(data["size"]["width"], 1, 1024, "width")
    height = integer(data["size"]["height"], 1, 1024, "height")
    require(data["surfaces"] == [{"id": "ground", "elevation": 0}],
            "unsupported surfaces")
    exact_keys(data["terrain"], {"encoding", "legend", "rows"})
    require(data["terrain"]["encoding"] == "rows", "unsupported terrain encoding")
    require(data["terrain"]["legend"] == {
        "G": "terrain.grass", "D": "terrain.dirt",
        "S": "terrain.stone", "W": "terrain.water"}, "unsupported terrain legend")
    rows = data["terrain"]["rows"]
    require(type(rows) is list and len(rows) == height, "invalid terrain row count")
    for row in rows:
        require(type(row) is str and len(row) == width and set(row) <= TERRAIN,
                "invalid terrain row")
    position(data["player_spawn"], width, height)
    require(data["region_exits"] == [], "region exits are not yet supported")
    placements = data["placements"]
    require(type(placements) is list and len(placements) <= 4096,
            "invalid placement list")
    ids, orders, spawn_ids = set(), set(), set()
    for placement in placements:
        exact_keys(placement, {"id", "kind", "definition_id",
                               "initialization", "position"}, {"spawn"})
        for field in ("id", "definition_id"):
            identity(placement[field], field)
        require(placement["id"] not in ids, "duplicate placement ID")
        ids.add(placement["id"])
        kind = placement["kind"]
        require(kind in KINDS, "unknown placement kind")
        exact_keys(placement["initialization"], {"stage", "order"})
        stage = placement["initialization"]["stage"]
        require(stage in STAGES, "unknown initialization stage")
        order = integer(placement["initialization"]["order"], 0, 4096, "order")
        require((stage, order) not in orders, "duplicate stage/order")
        orders.add((stage, order))
        position(placement["position"], width, height)
        if kind in {"npc_spawn", "monster_spawn"}:
            require("spawn" in placement, "NPC/monster missing spawn settings")
            spawn = placement["spawn"]
            exact_keys(spawn, {"spawn_id", "wander_radius",
                               "wander_interval_ticks", "maximum_active_count", "respawns"})
            identity(spawn["spawn_id"], "spawn_id")
            require(spawn["spawn_id"] not in spawn_ids, "duplicate spawn ID")
            spawn_ids.add(spawn["spawn_id"])
            for field in ("wander_radius", "wander_interval_ticks"):
                integer(spawn[field], 0, 2147483647, field)
            integer(spawn["maximum_active_count"], 1, 2147483647,
                    "maximum_active_count")
            require(type(spawn["respawns"]) is bool, "respawns must be boolean")
        else:
            require("spawn" not in placement, "resource/station cannot have spawn settings")
    return {"width": width, "height": height, "placements": len(placements)}


def load_snapshot(path):
    path = Path(path)
    require(path.stat().st_size <= MAX_BYTES, "snapshot exceeds 8 MiB limit")
    with path.open("r", encoding="utf-8") as stream:
        data = json.load(stream, object_pairs_hook=unique_object_pairs,
                         parse_constant=lambda value: (_ for _ in ()).throw(
                             SnapshotError("nonfinite number: " + value)))
    return validate(data)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    args = parser.parse_args(argv)
    try:
        result = load_snapshot(args.snapshot)
    except (OSError, UnicodeError, ValueError, TypeError, KeyError) as error:
        print(f"Snapshot rejected: {error}", file=sys.stderr)
        return 1
    print(f"Valid snapshot: {result['width']}x{result['height']}, "
          f"{result['placements']} placements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
