"""Real C++ editing/recovery and deterministic killed-process boundaries."""
from __future__ import annotations
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_world_editor import StudioBridge, StudioError
from embervale_recovery import RecoveryStore, RecoveryError, state_digest

TOOL: Path
PROBE: Path
FIXTURE: Path


def journal_child(tool: Path, source: Path, recovery: Path, phase: str):
    bridge = StudioBridge(tool, source)
    store = RecoveryStore(recovery)
    bridge.paint(40, 20, "S")
    writer = store.writer(bridge)
    writer.checkpoint(bridge)
    bridge.paint(41, 20, "S")
    if phase == "writing":
        import embervale_recovery
        replace = embervale_recovery.os.replace
        def pause(temporary, destination):
            print("paused:journal", flush=True)
            sys.stdin.read()
            replace(temporary, destination)
        embervale_recovery.os.replace = pause
        writer.checkpoint(bridge)
    else:
        writer.checkpoint(bridge)
        bridge.paint(42, 20, "S")  # An accepted but uncheckpointed later edit.
        print("paused:journal", flush=True)
        sys.stdin.read()
    raise RuntimeError("Interruption fixture reached normal exit")


class RecoveryIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="embervale-recovery-real-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source.json"
        self.source.write_bytes(FIXTURE.read_bytes())
        self.original = self.source.read_bytes()
        self.store = RecoveryStore(self.root / "recovery")
        self.bridge = StudioBridge(TOOL, self.source)

    def writer(self):
        result = self.store.writer(self.bridge)
        self.addCleanup(result.close)
        return result

    def killed_at(self, arguments, marker, while_paused=None):
        child = subprocess.Popen(arguments, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, text=True)
        ready = threading.Event()
        lines = []
        def read_ready():
            for line in child.stdout:
                lines.append(line)
                if line.strip() == marker:
                    ready.set()
                    return
        reader = threading.Thread(target=read_ready, daemon=True)
        reader.start()
        try:
            if not ready.wait(10):
                child.kill()
                child.wait(timeout=10)
                reader.join(timeout=1)
                raise AssertionError("Probe did not reach its boundary: " +
                                     "".join(lines) + child.stderr.read())
            if while_paused is not None:
                while_paused()
            child.kill()
            child.wait(timeout=10)
            self.assertNotEqual(child.returncode, 0)
        finally:
            if child.poll() is None:
                child.kill()
                child.wait(timeout=10)
            reader.join(timeout=1)
            for stream in (child.stdin, child.stdout, child.stderr):
                stream.close()

    def test_real_authority_replay_preserves_edits_settings_and_reserved_ids(self):
        bridge = self.bridge
        writer = self.writer()
        first = bridge.place("resource.normal_tree", 40, 20)
        writer.checkpoint(bridge)
        bridge.undo()
        writer.checkpoint(bridge)
        # Undone authored IDs must remain reserved across interruption/reopen.
        self.assertNotIn(first, bridge.placements)
        bridge.paint(41, 20, "S")
        writer.checkpoint(bridge)
        bridge.configure_spawn("monster.passive_development.001", 2, 3, 1, True, 6)
        writer.checkpoint(bridge)
        expected = copy.deepcopy(bridge.current_snapshot)
        writer.close()
        candidates = self.store.candidates(self.source)
        self.assertEqual(len(candidates), 3)
        self.assertIn("Changed terrain tiles: 1", self.store.preview(candidates[0], TOOL))
        restored = self.store.restore(candidates[0], TOOL, self.root / "recovered.json")
        self.assertEqual(restored.current_snapshot, expected)
        self.assertFalse(restored.dirty)
        self.assertEqual(restored.place("station.bank", 42, 20), "station.authored.000002")
        restored.undo()
        restored.redo()
        saved = self.root / "recovered-next.json"
        restored.save(saved)
        self.assertEqual(StudioBridge(TOOL, saved).current_snapshot, restored.current_snapshot)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.assertFalse((self.root / "saves").exists())
        self.assertEqual(self.store.candidates(self.source), [])

    def test_changed_base_is_never_overwritten_or_silently_merged(self):
        writer = self.writer()
        self.bridge.paint(40, 20, "S")
        writer.checkpoint(self.bridge)
        expected = copy.deepcopy(self.bridge.current_snapshot)
        writer.close()
        external = StudioBridge(TOOL, self.source)
        external.paint(41, 20, "D")
        changed = self.root / "external.json"
        external.save(changed)
        self.source.write_bytes(changed.read_bytes())
        original_now = self.source.read_bytes()
        candidate = self.store.candidates(self.source)[0]
        self.assertTrue(candidate.base_changed())
        with self.assertRaises(RecoveryError):
            self.store.restore(candidate, TOOL, self.source)
        destination = self.root / "separate folder"
        destination.mkdir()
        restored = self.store.restore(candidate, TOOL, destination / "recovered.json")
        self.assertEqual(restored.current_snapshot, expected)
        self.assertEqual(self.source.read_bytes(), original_now)

    def test_missing_and_corrupt_originals_can_recover_from_cached_base(self):
        for condition in ("missing", "corrupt"):
            with self.subTest(condition=condition):
                self.source.write_bytes(self.original)
                self.bridge = StudioBridge(TOOL, self.source)
                self.bridge.paint(40, 20, "S")
                expected = copy.deepcopy(self.bridge.current_snapshot)
                writer = self.writer()
                writer.checkpoint(self.bridge)
                writer.close()
                if condition == "missing":
                    self.source.unlink()
                else:
                    self.source.write_bytes(b"invalid world")
                candidate = self.store.candidates(self.source)[0]
                restored = self.store.restore(candidate, TOOL, self.root / (condition + "-recovered.json"))
                self.assertEqual(restored.current_snapshot, expected)
                self.assertEqual(self.source.exists(), condition == "corrupt")
                if condition == "corrupt":
                    self.assertEqual(self.source.read_bytes(), b"invalid world")

    def test_legacy_lock_is_preserved_and_does_not_block_separate_recovery(self):
        writer = self.writer()
        self.bridge.paint(40, 20, "S")
        writer.checkpoint(self.bridge)
        writer.close()
        legacy = Path(str(self.source) + ".embervale.lock")
        legacy.mkdir()
        (legacy / "unknown-writer").write_bytes(b"do not delete")
        with self.assertRaisesRegex(StudioError, "legacy save lock"):
            self.bridge.save(self.root / "blocked.json")
        candidate = self.store.candidates(self.source)[0]
        restored = self.store.restore(candidate, TOOL, self.root / "recovered.json")
        self.assertEqual(restored.terrain[20][40], "S")
        self.assertEqual((legacy / "unknown-writer").read_bytes(), b"do not delete")
        self.assertFalse((self.root / "blocked.json").exists())
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_occupied_destination_preserves_both_file_and_recovery_copies(self):
        writer = self.writer()
        self.bridge.paint(40, 20, "S")
        writer.checkpoint(self.bridge)
        writer.close()
        candidate = self.store.candidates(self.source)[0]
        destination = self.root / "existing.json"
        destination.write_bytes(b"private destination")
        with self.assertRaises(RecoveryError):
            self.store.restore(candidate, TOOL, destination)
        self.assertEqual(destination.read_bytes(), b"private destination")
        self.assertEqual(len(self.store.candidates(self.source)), 1)
        self.assertEqual(self.source.read_bytes(), self.original)
        self.store.restore(candidate, TOOL, self.root / "fresh.json")

    def test_runtime_invalid_newest_copy_can_fall_back_to_previous_generation(self):
        writer = self.writer()
        self.bridge.paint(40, 20, "S")
        writer.checkpoint(self.bridge)
        previous = copy.deepcopy(self.bridge.current_snapshot)
        self.bridge.paint(41, 20, "S")
        writer.checkpoint(self.bridge)
        writer.close()
        candidate = self.store.candidates(self.source)[0]
        packet = candidate.folder / candidate.filename
        envelope = json.loads(packet.read_bytes())
        envelope["payload"]["snapshot_sha256"] = "0" * 64
        envelope["sha256"] = state_digest(envelope["payload"])
        packet.write_text(json.dumps(envelope))
        candidates = self.store.candidates(self.source)
        with self.assertRaisesRegex(RecoveryError, "differs"):
            self.store.preview(candidates[0], TOOL)
        with self.assertRaisesRegex(RecoveryError, "differs"):
            self.store.restore(candidates[0], TOOL, self.root / "bad.json")
        self.assertFalse((self.root / "bad.json").exists())
        restored = self.store.restore(candidates[1], TOOL, self.root / "older.json")
        self.assertEqual(restored.current_snapshot, previous)

    def test_kill_during_journal_commit_keeps_last_complete_generation(self):
        self.killed_at([sys.executable, __file__, "--journal-child", str(TOOL),
            str(self.source), str(self.store.root), "writing"], "paused:journal")
        candidates = self.store.candidates(self.source)
        self.assertEqual(len(candidates), 1)
        self.assertTrue(list(candidates[0].folder.glob("*.tmp")))
        restored = self.store.restore(candidates[0], TOOL, self.root / "recovered.json")
        self.assertEqual(restored.terrain[20][40], "S")
        self.assertEqual(restored.terrain[20][41], self.bridge.terrain[20][41])
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_kill_after_checkpoint_reloads_it_not_later_live_edits(self):
        self.killed_at([sys.executable, __file__, "--journal-child", str(TOOL),
            str(self.source), str(self.store.root), "committed"], "paused:journal")
        restored = self.store.restore(self.store.candidates(self.source)[0], TOOL,
                                      self.root / "recovered.json")
        self.assertEqual(restored.terrain[20][40:42], "SS")
        self.assertEqual(restored.terrain[20][42], self.bridge.terrain[20][42])
        self.assertEqual(self.source.read_bytes(), self.original)

    def test_cpp_kill_at_lock_partial_stage_commit_and_published_boundaries(self):
        previous = self.root / "previous.json"
        previous.write_bytes(self.original + b"\n")
        prior_bytes = previous.read_bytes()
        for phase in ("locked", "writing", "staged", "published"):
            with self.subTest(phase=phase):
                target = self.root / (phase + ".json")
                self.killed_at([str(PROBE), str(self.source), str(target), phase], "paused:" + phase)
                self.assertEqual(target.exists(), phase == "published")
                if target.exists():
                    self.assertEqual(target.read_bytes(), self.original)
                    StudioBridge(TOOL, target)
                self.assertEqual(self.source.read_bytes(), self.original)
                self.assertEqual(previous.read_bytes(), prior_bytes)
                # The production writer can reuse the abandoned OS lock and
                # ignores unknown staging directories; no age-based deletion.
                saved = self.root / (phase + "-retry.json")
                self.bridge.save(saved)
                self.assertEqual(StudioBridge(TOOL, saved).current_snapshot, self.bridge.current_snapshot)
                if phase != "locked":
                    self.assertTrue(list(self.root.glob(".embervale-draft-*")))

    def test_live_cpp_writer_refuses_competitor_and_releases_after_kill(self):
        target = self.root / "owner.json"
        competitor = self.root / "competitor.json"
        def compete():
            with self.assertRaisesRegex(StudioError, "writer is active"):
                self.bridge.save(competitor)
            self.assertFalse(target.exists())
            self.assertFalse(competitor.exists())
            self.assertEqual(self.source.read_bytes(), self.original)
        self.killed_at([str(PROBE), str(self.source), str(target), "locked"], "paused:locked", compete)
        self.bridge.save(competitor)
        self.assertEqual(StudioBridge(TOOL, competitor).current_snapshot, self.bridge.current_snapshot)


def main():
    if len(sys.argv) == 6 and sys.argv[1] == "--journal-child":
        journal_child(Path(sys.argv[2]), Path(sys.argv[3]), Path(sys.argv[4]), sys.argv[5])
    parser = argparse.ArgumentParser()
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    args, remainder = parser.parse_known_args()
    global TOOL, PROBE, FIXTURE
    TOOL, PROBE, FIXTURE = args.tool.resolve(), args.probe.resolve(), args.fixture.resolve()
    unittest.main(argv=[sys.argv[0], *remainder])


if __name__ == "__main__":
    main()
