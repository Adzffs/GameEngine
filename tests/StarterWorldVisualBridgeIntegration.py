"""Executable-backed Embervale Studio regression checks.

These tests intentionally require the *built* EmbervaleWorldMove executable and
an actual, schema-valid 100x100 starter-world fixture. Unlike the mocked bridge
contract tests, they exercise the C++ world importer, history, CLI session,
validated publication, and Python front end together. No live/player save is used.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

# Paths are supplied by CMake, never silently replaced with a fake backend.
TOOL: Path
FIXTURE: Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_world_editor import StudioBridge, StudioError  # noqa: E402

TREE = "resource.normal_tree.001"
FURNACE = "station.furnace.001"


class RealWorldBridgeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = tempfile.TemporaryDirectory(prefix="embervale-bridge-")
        self.addCleanup(self.directory.cleanup)
        self.source = Path(self.directory.name) / "source.json"
        shutil.copyfile(FIXTURE, self.source)
        self.original = self.source.read_bytes()
        self.bridge = StudioBridge(TOOL, self.source)

    def test_source_matches_real_import_and_move_history(self) -> None:
        self.assertEqual(self.bridge.width, 100)
        self.assertEqual(self.bridge.height, 100)
        self.assertEqual(self.bridge.spawn, (0, 0))
        self.assertEqual(self.bridge.placements[TREE][1:], (5, 5))
        self.assertEqual(self.bridge.placements[FURNACE][1:], (14, 9))
        self.assertEqual(self.bridge.hash, hashlib.sha256(self.original).hexdigest())
        self.assertEqual(self.bridge.commands, [])
        self.bridge.move(TREE, 30, 30)
        self.assertEqual(self.bridge.placements[TREE][1:], (30, 30))
        self.assertEqual(self.bridge.status.split("/"), ["yes", "1", "0"])
        self.bridge.undo()
        self.assertEqual(self.bridge.placements[TREE][1:], (5, 5))
        self.bridge.redo()
        self.assertEqual(self.bridge.placements[TREE][1:], (30, 30))
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_paint_is_reversible_and_new_edit_discards_redo(self) -> None:
        self.assertEqual(self.bridge.terrain[30][31], "G")
        self.bridge.move(TREE, 30, 30)
        self.bridge.paint(31, 30, "D")
        self.assertEqual(self.bridge.terrain[30][31], "D")
        self.assertEqual(self.bridge.status.split("/"), ["yes", "2", "0"])
        # Backend no-op must not introduce a frontend replay command.
        previous_count = len(self.bridge.commands)
        self.bridge.paint(31, 30, "D")
        self.assertEqual(len(self.bridge.commands), previous_count)
        self.bridge.undo()
        self.assertEqual(self.bridge.terrain[30][31], "G")
        self.assertEqual(self.bridge.placements[TREE][1:], (30, 30))
        self.bridge.redo()
        self.assertEqual(self.bridge.terrain[30][31], "D")
        self.bridge.undo()
        self.bridge.paint(31, 30, "S")
        self.assertEqual(self.bridge.terrain[30][31], "S")
        self.assertEqual(self.bridge.status.split("/"), ["yes", "2", "0"])
        with self.assertRaises(StudioError):
            self.bridge.redo()
        self.assertEqual(self.bridge.terrain[30][31], "S")
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_rejected_paint_does_not_change_history(self) -> None:
        terrain = self.bridge.terrain[:]
        placements = dict(self.bridge.placements)
        # (0,0) is the canonical player spawn; water would invalidate it.
        with self.assertRaises(StudioError):
            self.bridge.paint(0, 0, "W")
        with self.assertRaises(StudioError):
            self.bridge.paint(5, 5, "W")  # existing tree
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.terrain, terrain)
        self.assertEqual(self.bridge.placements, placements)
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_draft_is_new_valid_sibling_and_reopens(self) -> None:
        self.bridge.move(TREE, 30, 30)
        self.bridge.paint(31, 30, "D")
        draft = self.source.with_name("world draft with spaces.json")
        self.assertEqual(self.bridge.save(draft), str(draft))
        data = draft.read_bytes()
        self.assertNotEqual(data, self.original)
        validated = subprocess.run(
            [str(TOOL), "--hash", str(draft)], capture_output=True,
            text=True, encoding="utf-8", timeout=20, check=False,
        )
        self.assertEqual(validated.returncode, 0, validated.stderr)
        self.assertEqual(validated.stdout.strip(), hashlib.sha256(data).hexdigest())
        reopened = StudioBridge(TOOL, draft)
        self.assertEqual(reopened.placements[TREE][1:], (30, 30))
        self.assertEqual(reopened.terrain[30][31], "D")
        self.assertEqual(reopened.commands, [])
        self.assertEqual(self.source.read_bytes(), self.original)
        with self.assertRaises(StudioError):
            self.bridge.save(draft)
        self.assertEqual(draft.read_bytes(), data)

    def test_raw_terminal_commands_cannot_bypass_guarded_draft_save(self) -> None:
        draft = self.source.with_name('unverified writer bypass.json')
        for command in (f'save {draft}', 'status', 'list', 'terrain', 'quit'):
            with self.subTest(command=command):
                with self.assertRaisesRegex(StudioError, 'Only move, paint, undo and redo'):
                    self.bridge.perform(command)
        self.assertFalse(draft.exists())
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.source.read_bytes(), self.original)
        # The separate verified save API continues to work as intended.
        self.bridge.save(draft)
        self.assertTrue(draft.is_file())

    def test_source_revision_conflict_prevents_edits_and_save(self) -> None:
        self.source.write_bytes(self.original + b"\n")
        with self.assertRaisesRegex(StudioError, "CONTENT_CONFLICT"):
            self.bridge.move(TREE, 30, 30)
        draft = self.source.with_name("must-not-exist.json")
        with self.assertRaisesRegex(StudioError, "CONTENT_CONFLICT"):
            self.bridge.save(draft)
        self.assertFalse(draft.exists())
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.placements[TREE][1:], (5, 5))
        self.assertEqual(self.source.read_bytes(), self.original + b"\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, type=Path,
                        help="Previously built EmbervaleWorldMove executable")
    parser.add_argument("--fixture", required=True, type=Path,
                        help="Real development starter snapshot fixture")
    options = parser.parse_args()
    TOOL = options.tool.resolve()
    FIXTURE = options.fixture.resolve()
    if not TOOL.is_file() or not FIXTURE.is_file():
        parser.error("the built tool and the starter snapshot must both exist")
    if not json.loads(FIXTURE.read_text(encoding="utf-8"))["size"] == {"width": 100, "height": 100}:
        parser.error("expected the real 100x100 development snapshot fixture")
    unittest.main(argv=[sys.argv[0]], verbosity=2)
