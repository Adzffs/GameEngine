#!/usr/bin/env python3
"""Regression tests for the strict read-only world snapshot validator."""
import copy
import json
import tempfile
import unittest
from pathlib import Path

from validate_world_snapshot import SnapshotError, load_snapshot, validate

FIXTURE = (Path(__file__).resolve().parent.parent /
           "tests/fixtures/content/development-starter.snapshot.json")


class SnapshotValidationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.snapshot = json.loads(FIXTURE.read_text(encoding="utf-8"))

    def test_compiled_world_fixture(self):
        self.assertEqual(validate(self.snapshot),
                         {"width": 100, "height": 100, "placements": 20})
        self.assertEqual(load_snapshot(FIXTURE)["placements"], 20)

    def rejects(self, mutate):
        data = copy.deepcopy(self.snapshot)
        mutate(data)
        with self.assertRaises((SnapshotError, TypeError, KeyError)):
            validate(data)

    def test_rejects_bad_schema(self):
        self.rejects(lambda d: d.update(schema_version=2))

    def test_rejects_missing_row(self):
        self.rejects(lambda d: d["terrain"]["rows"].pop())

    def test_rejects_invalid_terrain(self):
        self.rejects(lambda d: d["terrain"]["rows"].__setitem__(0, "X" * 100))

    def test_rejects_out_of_bounds_position(self):
        self.rejects(lambda d: d["placements"][0]["position"].update(x=100))

    def test_rejects_duplicate_placement_id(self):
        self.rejects(lambda d: d["placements"][1].update(
            id=d["placements"][0]["id"]))

    def test_rejects_duplicate_initialization_order(self):
        self.rejects(lambda d: d["placements"][1].update(
            initialization=copy.deepcopy(d["placements"][0]["initialization"])))

    def test_rejects_unknown_field(self):
        self.rejects(lambda d: d.update(unsafe_override=True))

    def test_rejects_missing_spawn_settings(self):
        self.rejects(lambda d: d["placements"][0].pop("spawn"))

    def test_rejects_boolean_as_integer(self):
        self.rejects(lambda d: d["size"].update(width=True))

    def test_rejects_unsupported_surface(self):
        self.rejects(lambda d: d["player_spawn"].update(surface_id="upper"))

    def test_rejects_duplicate_json_keys(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bad.json"
            path.write_text('{"format": "a", "format": "b"}', encoding="utf-8")
            with self.assertRaises(SnapshotError):
                load_snapshot(path)

    def test_rejects_oversized_file(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "large.json"
            with path.open("wb") as stream:
                stream.truncate(8 * 1024 * 1024 + 1)
            with self.assertRaises(SnapshotError):
                load_snapshot(path)


if __name__ == "__main__":
    unittest.main()
