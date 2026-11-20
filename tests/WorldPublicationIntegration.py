"""Versioned publication against real C++ tools and killed writers."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from embervale_world_editor import StudioBridge
from embervale_publication import PublicationClient, PublicationError
from embervale_recovery import state_digest

PUBLISHER:Path
EDITOR:Path
PROBE:Path
FIXTURE:Path


class PublicationIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix="embervale-world-version-")
        self.addCleanup(self.temporary.cleanup)
        self.root=Path(self.temporary.name)
        self.source=self.root/"draft.json";self.source.write_bytes(FIXTURE.read_bytes())
        self.original=self.source.read_bytes()
        self.bridge=StudioBridge(EDITOR,self.source)
        self.client=PublicationClient(PUBLISHER)
        self.library=self.root/"versions"

    def run_tool(self,*arguments):
        return subprocess.run([str(PUBLISHER),*map(str,arguments)],capture_output=True,text=True,timeout=20)

    def publish(self,name="v1"):
        self.client.validate(self.bridge)
        return self.client.publish(self.bridge,self.library,name)

    def paint_edit(self):
        for y,row in enumerate(self.bridge.terrain):
            for x,glyph in enumerate(row):
                if glyph=="G":
                    self.bridge.paint(x,y,"S")
                    return

    def test_validate_is_read_only_and_receipt_matches_raw_input(self):
        before=set(self.root.iterdir())
        value=self.client.validate(self.bridge)
        self.assertEqual(value["source_sha256"],hashlib.sha256(self.original).hexdigest())
        self.assertEqual(before,set(self.root.iterdir()))
        self.assertFalse(self.library.exists())

    def test_publish_reopen_and_rollback_select_exact_immutable_versions(self):
        first=self.publish()
        first_bytes=(first.manifest.read_bytes(),(first.manifest.parent/"world.json").read_bytes())
        self.paint_edit()
        self.bridge.save(self.root/"edited.json")
        second=self.publish("v2")
        self.assertNotEqual(first.metadata["world_sha256"],second.metadata["world_sha256"])
        self.assertEqual([p.parent.name for p in self.client.versions(self.library)],["v1","v2"])
        selected=StudioBridge(EDITOR,second.manifest.parent/"world.json")
        self.assertEqual(selected.current_snapshot,self.bridge.current_snapshot)
        rollback=self.client.inspect(first.manifest)
        self.assertEqual(rollback.metadata,first.metadata)
        self.assertEqual(first_bytes,(first.manifest.read_bytes(),(first.manifest.parent/"world.json").read_bytes()))
        self.assertEqual(self.source.read_bytes(),self.original)
        self.assertFalse((self.root/"saves").exists())

    def test_name_collision_and_bad_names_leave_versions_intact(self):
        first=self.publish()
        before=first.manifest.read_bytes()
        for name in ("v1","../escape","CON","con","trailing-"):
            with self.subTest(name=name),self.assertRaises(PublicationError):
                self.client.publish(self.bridge,self.library,name)
        self.assertEqual(first.manifest.read_bytes(),before)
        self.assertFalse((self.root/"escape").exists())

    def test_dirty_and_changed_draft_invalidate_old_receipt(self):
        self.client.validate(self.bridge)
        self.source.write_bytes(self.original+b"\n")
        with self.assertRaises(Exception):
            self.client.publish(self.bridge,self.library,"stale")
        self.assertFalse(self.library.exists())
        self.source.write_bytes(self.original)
        self.client.validate(self.bridge)
        self.paint_edit()
        with self.assertRaises(PublicationError):
            self.client.publish(self.bridge,self.library,"dirty")
        self.assertFalse(self.library.exists())

    def test_changed_validator_receipt_rejects_before_writes(self):
        value=self.client.validate(self.bridge)
        value["validator_revision"]="0"*64
        receipt=self.root/"stale-receipt.json";receipt.write_text(json.dumps(value))
        result=self.run_tool("--publish",self.source,"--receipt",receipt,"--library",self.library,"--revision","v1")
        self.assertNotEqual(result.returncode,0)
        self.assertIn("validation tool changed",result.stderr)
        self.assertFalse(self.library.exists())

    def test_payload_mutation_and_manifest_traversal_refuse(self):
        first=self.publish()
        world=first.manifest.parent/"world.json"
        world.write_bytes(world.read_bytes()+b"\n")
        with self.assertRaisesRegex(PublicationError,"SHA-256"):
            self.client.inspect(first.manifest)
        manifest=json.loads(first.manifest.read_bytes());manifest["world_file"]="../../draft.json"
        first.manifest.write_text(json.dumps(manifest))
        with self.assertRaises(PublicationError):
            self.client.inspect(first.manifest)
        self.assertEqual(self.source.read_bytes(),self.original)

    def test_forged_matching_hash_cannot_bypass_current_runtime_validation(self):
        first=self.publish()
        world=first.manifest.parent/"world.json"
        data=json.loads(world.read_bytes())
        x,y=data["player_spawn"]["x"],data["player_spawn"]["y"]
        rows=data["terrain"]["rows"];rows[y]=rows[y][:x]+"W"+rows[y][x+1:]
        world.write_text(json.dumps(data))
        manifest=json.loads(first.manifest.read_bytes())
        manifest["world_sha256"]=hashlib.sha256(world.read_bytes()).hexdigest()
        manifest["validator_revision"]="0"*64
        first.manifest.write_text(json.dumps(manifest))
        with self.assertRaises(PublicationError):
            self.client.inspect(first.manifest)

    def test_invalid_route_never_creates_version_library(self):
        data=json.loads(self.original)
        x,y=data["player_spawn"]["x"],data["player_spawn"]["y"]
        rows=data["terrain"]["rows"];rows[y]=rows[y][:x]+"W"+rows[y][x+1:]
        invalid=self.root/"invalid.json";invalid.write_text(json.dumps(data))
        result=self.run_tool("--validate",invalid)
        self.assertNotEqual(result.returncode,0)
        self.assertFalse(self.library.exists())
        self.assertEqual(self.source.read_bytes(),self.original)

    def kill_at(self,source,revision,phase,compete=None):
        child=subprocess.Popen([str(PROBE),str(source),str(self.library),revision,phase],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        ready=threading.Event();lines=[]
        def reader():
            for line in child.stdout:
                lines.append(line)
                if line.strip()=="paused:"+phase:
                    ready.set();return
        thread=threading.Thread(target=reader,daemon=True);thread.start()
        try:
            if not ready.wait(10):
                child.kill();child.wait(timeout=10);thread.join(timeout=1)
                raise AssertionError("Writer never reached boundary: "+"".join(lines)+child.stderr.read())
            if compete:compete()
            child.kill();child.wait(timeout=10)
            self.assertNotEqual(child.returncode,0)
        finally:
            if child.poll() is None:child.kill();child.wait(timeout=10)
            thread.join(timeout=1)
            for stream in (child.stdin,child.stdout,child.stderr):stream.close()

    def test_killed_publication_is_committed_only_when_manifest_exists(self):
        prior=self.publish("prior");before=prior.manifest.read_bytes()
        for phase in ("reserved","world-written","committed"):
            with self.subTest(phase=phase):
                revision="kill-"+phase
                self.kill_at(self.source,revision,phase)
                manifest=self.library/revision/"manifest.json"
                self.assertEqual(manifest.exists(),phase=="committed")
                listed=self.client.versions(self.library)
                self.assertEqual(manifest in listed,phase=="committed")
                if phase=="committed":self.client.inspect(manifest)
                else:
                    with self.assertRaises(PublicationError):
                        self.client.publish(self.bridge,self.library,revision)
                self.assertEqual(prior.manifest.read_bytes(),before)
                self.assertEqual(self.source.read_bytes(),self.original)
        self.publish("after-interruption")

    def test_concurrent_name_reservation_and_owner_death_do_not_replace_data(self):
        other=self.root/"other.json";other.write_bytes(self.original)
        second=PublicationClient(PUBLISHER);bridge=StudioBridge(EDITOR,other);second.validate(bridge)
        def compete():
            with self.assertRaises(PublicationError):
                second.publish(bridge,self.library,"same-name")
        self.kill_at(self.source,"same-name","reserved",compete)
        self.assertFalse((self.library/"same-name/manifest.json").exists())
        self.assertEqual(self.source.read_bytes(),self.original)
        second.publish(bridge,self.library,"fresh-name")
        self.assertEqual(len(self.client.versions(self.library)),1)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--publisher",type=Path,required=True)
    parser.add_argument("--editor",type=Path,required=True)
    parser.add_argument("--probe",type=Path,required=True)
    parser.add_argument("--fixture",type=Path,required=True)
    args,rest=parser.parse_known_args()
    global PUBLISHER,EDITOR,PROBE,FIXTURE
    PUBLISHER,EDITOR,PROBE,FIXTURE=(p.resolve() for p in (args.publisher,args.editor,args.probe,args.fixture))
    unittest.main(argv=[sys.argv[0],*rest])

if __name__=="__main__":main()
