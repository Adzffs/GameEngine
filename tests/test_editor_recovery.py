"""Journal integrity, ownership and retention tests; no display/backend required."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_recovery import RecoveryStore, RecoveryError, FileLock, state_digest


class RecoveryJournalTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="embervale-recovery-unit-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source.json"
        self.source.write_bytes(b'{"private":"base"}')
        self.store = RecoveryStore(self.root / "recovery")
        self.bridge = SimpleNamespace(source=self.source, original_bytes=self.source.read_bytes(),
            hash=hashlib.sha256(self.source.read_bytes()).hexdigest(),
            commands=[], current_snapshot={"revision": 0}, dirty=True)

    def writer(self):
        writer = self.store.writer(self.bridge)
        self.addCleanup(writer.close)
        return writer

    def edit(self, writer, count):
        self.bridge.commands.append(f"paint {count} 1 S")
        self.bridge.current_snapshot = {"revision": count}
        writer.checkpoint(self.bridge)

    def test_store_is_lazy_and_repeated_display_does_not_write(self):
        self.assertFalse(self.store.root.exists())
        writer = self.writer()
        self.edit(writer, 1)
        def contents():
            # Windows byte-range locks also exclude readers. Inspect the live
            # lock's identity/metadata without opening its locked byte range.
            return {p.name: (p.stat().st_ino, p.stat().st_size, p.stat().st_mtime_ns,
                             None if p.name == "owner.lock" else p.read_bytes())
                    for p in writer.folder.iterdir()}
        before = contents()
        writer.checkpoint(self.bridge)
        self.assertEqual(before, contents())

    def test_three_generations_and_live_window_isolation(self):
        first = self.writer()
        for n in range(1, 6):
            self.edit(first, n)
        second = self.writer()
        second.checkpoint(self.bridge)
        self.assertEqual(self.store.candidates(self.source), [])
        first.close()
        copies = self.store.candidates(self.source)
        self.assertEqual([c.payload["sequence"] for c in copies], [5, 4, 3])
        self.assertTrue(all(c.folder == first.folder for c in copies))
        second.close()
        self.assertEqual(len(self.store.candidates(self.source)), 4)

    def test_partial_and_corrupt_newest_leave_previous_valid_copy(self):
        writer = self.writer()
        self.edit(writer, 1)
        self.edit(writer, 2)
        newest = sorted(writer.folder.glob("[0-9]*.json"))[-1]
        newest.write_bytes(b'{"partial":')
        (writer.folder / ("a" * 32 + ".tmp")).write_bytes(b"incomplete")
        writer.close()
        copies = self.store.candidates(self.source)
        self.assertEqual(len(copies), 1)
        self.assertEqual(copies[0].payload["snapshot_sha256"], state_digest({"revision": 1}))
        self.assertTrue(self.store.diagnostics)

    def test_failed_commit_preserves_previous_bytes_and_retries_latest_state(self):
        writer = self.writer()
        self.edit(writer, 1)
        original = next(writer.folder.glob("[0-9]*.json"))
        before = original.read_bytes()
        with patch("embervale_recovery.os.replace", side_effect=OSError("storage unavailable")):
            with self.assertRaises(OSError):
                self.edit(writer, 2)
        self.assertEqual(original.read_bytes(), before)
        writer.checkpoint(self.bridge)
        writer.close()
        self.assertEqual(self.store.candidates(self.source)[0].payload["snapshot_sha256"],
                         state_digest({"revision": 2}))

    def test_storage_limit_keeps_last_valid_copy(self):
        writer = self.writer()
        self.edit(writer, 1)
        original = next(writer.folder.glob("[0-9]*.json")).read_bytes()
        with patch("embervale_recovery.MAX_STORE", 1):
            with self.assertRaisesRegex(RecoveryError, "32 MiB"):
                self.edit(writer, 2)
        self.assertEqual(next(writer.folder.glob("[0-9]*.json")).read_bytes(), original)

    def test_resolved_session_is_terminal_and_only_its_files_are_collected(self):
        first = self.writer()
        self.edit(first, 1)
        first.close()
        second = self.writer()
        second.checkpoint(self.bridge)
        second.close()
        candidate = next(c for c in self.store.candidates(self.source) if c.folder == first.folder)
        self.store.resolve(candidate)
        self.assertTrue(first.folder.exists())
        self.assertEqual([c.folder for c in self.store.candidates(self.source)], [second.folder])
        third = self.writer()
        self.assertFalse(first.folder.exists())
        self.assertTrue(second.folder.exists())
        third.resolve()

    def test_copy_cannot_be_discarded_while_owner_is_alive(self):
        writer = self.writer()
        self.edit(writer, 1)
        writer.close()
        candidate = self.store.candidates(self.source)[0]
        with FileLock(candidate.folder / "owner.lock"):
            with self.assertRaises(RecoveryError):
                self.store.resolve(candidate)
        self.assertTrue((candidate.folder / candidate.filename).exists())

    def test_changed_or_missing_source_is_flagged_without_mutation(self):
        writer = self.writer()
        self.edit(writer, 1)
        writer.close()
        candidate = self.store.candidates(self.source)[0]
        self.assertFalse(candidate.base_changed())
        self.source.write_bytes(b"external changed bytes")
        self.assertTrue(candidate.base_changed())
        self.source.unlink()
        self.assertTrue(candidate.base_changed())
        self.assertEqual(len(self.store.candidates(self.source)), 1)

    def test_tampered_commands_and_identity_are_rejected_even_with_new_checksum(self):
        for change in [
            {"commands": ["recover-copy injected.json"]},
            {"commands": ["paint 1 1 S\nsave injected.json"]},
            {"commands": ["undo"] * 257},
            {"source": str(self.root / "other.json")},
            {"base_sha256": "0" * 64},
            {"format": 999},
        ]:
            with self.subTest(change=change):
                writer = self.writer()
                self.bridge.commands = []
                self.edit(writer, 1)
                packet = next(writer.folder.glob("[0-9]*.json"))
                envelope = json.loads(packet.read_bytes())
                envelope["payload"].update(change)
                envelope["sha256"] = state_digest(envelope["payload"])
                packet.write_text(json.dumps(envelope))
                writer.close()
                self.assertFalse(any(c.folder == writer.folder for c in self.store.candidates(self.source)))
        self.assertEqual(self.source.read_bytes(), b'{"private":"base"}')

    def test_unknown_files_and_symlinks_are_preserved(self):
        writer = self.writer()
        self.edit(writer, 1)
        outside = self.root / "outside.json"
        outside.write_bytes(b"outside")
        packet = next(writer.folder.glob("[0-9]*.json"))
        packet.unlink()
        try:
            packet.symlink_to(outside)
        except OSError:
            self.skipTest("symlinks unavailable")
        writer.close()
        self.assertEqual(self.store.candidates(self.source), [])
        self.assertEqual(outside.read_bytes(), b"outside")
        link = self.root / "linked-recovery"
        link.symlink_to(self.store.root, target_is_directory=True)
        with self.assertRaises(RecoveryError):
            RecoveryStore(link)

    def test_corrupt_resolution_does_not_authorize_cleanup(self):
        writer = self.writer()
        self.edit(writer, 1)
        packet = next(writer.folder.glob("[0-9]*.json"))
        (writer.folder / "resolved.json").write_bytes(b"partial receipt")
        writer.close()
        self.assertEqual(self.store.candidates(self.source), [])
        another = self.writer()
        self.assertTrue(packet.exists())
        self.assertTrue(writer.folder.exists())
        another.resolve()


if __name__ == "__main__":
    unittest.main()
