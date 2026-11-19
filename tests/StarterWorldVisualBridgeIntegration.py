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
from unittest.mock import patch, Mock
from pathlib import Path

# Paths are supplied by CMake, never silently replaced with a fake backend.
TOOL: Path
FIXTURE: Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_world_editor import StudioBridge, StudioError, DraftPlaytest  # noqa: E402

TREE = "resource.normal_tree.001"
FURNACE = "station.furnace.001"


class RealWorldBridgeTests(unittest.TestCase):
    def test_create_remove_save_reopen_and_allocate_again(self) -> None:
        tree = self.bridge.place("resource.normal_tree", 40, 20)
        furnace = self.bridge.place("station.furnace", 42, 20)
        self.assertEqual(tree, "resource.authored.000001")
        self.assertEqual(furnace, "station.authored.000002")
        self.assertEqual(self.bridge.definitions[tree], "resource.normal_tree")
        self.bridge.move(tree, 41, 20)
        self.bridge.paint(40, 20, "S")
        self.bridge.remove(TREE)
        self.assertNotIn(TREE, self.bridge.placements)
        self.bridge.undo()
        self.assertIn(TREE, self.bridge.placements)
        self.bridge.redo()
        draft = self.source.with_name("placed and removed objects.json")
        self.bridge.save(draft)
        self.assertFalse(self.bridge.dirty)
        reopened = StudioBridge(TOOL, draft)
        self.assertEqual(reopened.placements, self.bridge.placements)
        self.assertEqual(reopened.current_snapshot["schema_version"], 2)
        reopened.remove(tree)
        next_id = reopened.place("resource.copper_rock", 41, 20)
        self.assertEqual(next_id, "resource.authored.000003")
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_undone_and_removed_ids_remain_reserved_after_reopening(self) -> None:
        first = self.bridge.place("resource.normal_tree", 40, 20)
        self.bridge.undo()
        self.assertNotIn(first, self.bridge.placements)
        self.assertTrue(self.bridge.dirty)  # allocation reservation must be saved
        self.bridge.redo()
        self.assertIn(first, self.bridge.placements)
        self.bridge.undo()
        second = self.bridge.place("station.bank", 40, 20)
        self.assertEqual(second, "station.authored.000002")
        with self.assertRaises(StudioError):
            self.bridge.redo()
        self.bridge.remove(second)
        draft = self.source.with_name("allocation reservations.json")
        self.bridge.save(draft)
        reopened = StudioBridge(TOOL, draft)
        self.assertEqual(reopened.place("resource.oak_tree", 40, 20), "resource.authored.000003")

    def test_creation_rejection_preserves_allocator_history_and_actor_spawns(self) -> None:
        for x, y in ((0, 0), (5, 5), (10, 10)):
            with self.assertRaises(StudioError):
                self.bridge.place("resource.normal_tree", x, y)
        with self.assertRaises(StudioError):
            self.bridge.remove("npc.development_guide.001")
        with self.assertRaises(StudioError):
            self.bridge.perform("remove npc.development_guide.001")
        self.assertEqual(self.bridge.commands, [])
        self.assertEqual(self.bridge.current_snapshot["schema_version"], 1)
        self.assertFalse(self.bridge.dirty)
        self.assertEqual(self.bridge.place("station.crafting_table", 40, 20), "station.authored.000001")
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_playtest_validates_private_copy_and_never_passes_save_path(self) -> None:
        self.bridge.move(TREE, 30, 30)
        runner = DraftPlaytest()
        with self.assertRaisesRegex(StudioError, "Save a new draft"):
            runner.start(self.bridge, TOOL)
        draft = self.source.with_name("play draft.json")
        self.bridge.save(draft)
        process = Mock()
        process.poll.return_value = None
        real_popen = subprocess.Popen
        launched = []
        def spawn_game(arguments, **kwargs):
            if "--session" in arguments:
                return real_popen(arguments, **kwargs)
            launched.append(arguments)
            return process
        with patch("embervale_world_editor.subprocess.Popen", side_effect=spawn_game):
            runner.start(self.bridge, TOOL)
            arguments = launched[0]
            self.assertEqual(arguments[0], str(TOOL))
            self.assertEqual(arguments[1], "--world")
            self.assertEqual(arguments[3:], ["--no-save"])
            private = Path(arguments[2])
            self.assertNotEqual(private, draft)
            self.assertEqual(private.read_bytes(), draft.read_bytes())
            with self.assertRaisesRegex(StudioError, "already running"):
                runner.start(self.bridge, TOOL)
            self.assertEqual(len(launched), 1)
        process.poll.return_value = 0
        self.assertEqual(runner.poll(), 0)
        self.assertFalse(private.exists())
        self.assertEqual(self.source.read_bytes(), self.original)
        draft.write_bytes(self.original)
        with self.assertRaisesRegex(StudioError, "Saved draft differs"):
            runner.start(self.bridge, TOOL)

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
                with self.assertRaisesRegex(StudioError, 'Only move, paint, place, remove, undo and redo'):
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
