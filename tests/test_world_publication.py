"""Headless metadata, library-browser and published-launch contract tests."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from embervale_publication import metadata, PublicationError, PublicationClient, PublishedVersion, valid_revision
from embervale_world_editor import DraftPlaytest, StudioError


def receipt():
    return dict(receipt_version=1, source_sha256="a"*64, world_sha256="b"*64,
                world_schema_version=4, validator_profile="starter-runtime-v1", validator_revision="c"*64)


class PublicationContractTests(unittest.TestCase):
    def test_receipt_and_manifest_shapes(self):
        value = receipt()
        self.assertEqual(metadata(json.dumps(value)), value)
        del value["receipt_version"]
        value.update(manifest_version=1, revision="river-v1", world_file="world.json")
        self.assertEqual(metadata(json.dumps(value), manifest=True), value)

    def test_duplicate_and_unknown_fields_reject(self):
        with self.assertRaises(PublicationError):
            metadata(json.dumps(receipt()).replace('"receipt_version": 1', '"receipt_version": 1, "receipt_version": 1'))
        value = receipt(); value["unknown"] = 1
        with self.assertRaises(PublicationError):
            metadata(json.dumps(value))

    def test_byte_limit_and_invalid_json_reject(self):
        for raw in (" "*4097, '{"partial":', "null", "[]"):
            with self.subTest(raw=raw[:20]), self.assertRaises(PublicationError):
                metadata(raw)

    def test_wrong_version_and_boolean_integer_reject(self):
        for change in ({"receipt_version": True}, {"receipt_version": 2}, {"world_schema_version": True}):
            value=receipt();value.update(change)
            with self.assertRaises(PublicationError):
                metadata(json.dumps(value))

    def test_unsupported_profile_and_hashes_reject(self):
        for change in ({"validator_profile":"future"}, {"world_sha256":".."}, {"validator_revision":"G"*64}):
            value=receipt();value.update(change)
            with self.assertRaises(PublicationError):
                metadata(json.dumps(value))

    def test_portable_revision_names(self):
        for name in ("", ".", "..", "../escape", "a/b", "a b", "CAPS", "a"*49, "con", "com1", "-bad", "bad-"):
            self.assertFalse(valid_revision(name),name)
        for name in ("v1","river-crossing-v2","a"*48):
            self.assertTrue(valid_revision(name))

    def test_manifest_paths_and_schema_reject(self):
        value=receipt();del value["receipt_version"]
        value.update(manifest_version=1, revision="v1", world_file="../outside")
        with self.assertRaises(PublicationError):
            metadata(json.dumps(value),manifest=True)
        value["world_file"]="world.json";value["world_schema_version"]=5
        with self.assertRaises(PublicationError):
            metadata(json.dumps(value),manifest=True)

    def test_browser_excludes_partial_and_linked_versions(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            (root/"v1").mkdir();(root/"v1/manifest.json").write_text("{}")
            (root/"partial").mkdir();(root/"partial/world.json").write_text("{}")
            self.assertEqual(PublicationClient.versions(root),[root/"v1/manifest.json"])
            try:(root/"linked").symlink_to(root/"v1",target_is_directory=True)
            except OSError:return
            self.assertEqual(PublicationClient.versions(root),[root/"v1/manifest.json"])

    def test_missing_library_is_read_only(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"absent"
            self.assertEqual(PublicationClient.versions(path),[])
            self.assertFalse(path.exists())

    def test_published_game_arguments_are_pinned_and_temporary(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);game=root/"game";game.write_text("placeholder")
            manifest=root/"versions/v1/manifest.json"
            publisher=Mock();publisher.inspect.return_value=PublishedVersion(manifest,{})
            process=Mock();process.poll.return_value=None
            player=DraftPlaytest()
            with patch("embervale_world_editor.subprocess.Popen",return_value=process) as spawn:
                player.start_version(publisher,manifest,game)
                self.assertEqual(spawn.call_args.args[0],[str(game.resolve()),"--world-version",str(manifest),"--no-save","--profile","fresh"])
                work=Path(spawn.call_args.kwargs["cwd"])
                self.assertTrue(work.is_dir());self.assertNotEqual(work,manifest.parent)
                with self.assertRaisesRegex(StudioError,"already running"):
                    player.start_version(publisher,manifest,game)
                self.assertEqual(spawn.call_count,1)
            process.poll.return_value=0;self.assertEqual(player.poll(),0)
            self.assertFalse(work.exists());self.assertFalse((root/"saves").exists())

    def test_bad_profile_and_bad_version_never_launch(self):
        with tempfile.TemporaryDirectory() as d:
            game=Path(d)/"game";game.write_text("placeholder")
            publisher=Mock();publisher.inspect.side_effect=PublicationError("changed version")
            player=DraftPlaytest()
            with patch("embervale_world_editor.subprocess.Popen") as spawn:
                with self.assertRaises(StudioError):player.start_version(publisher,Path(d)/"manifest.json",game,"invalid")
                with self.assertRaises(PublicationError):player.start_version(publisher,Path(d)/"manifest.json",game)
                spawn.assert_not_called()


if __name__ == "__main__":
    unittest.main()
