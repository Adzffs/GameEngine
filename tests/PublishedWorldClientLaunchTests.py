"""Launch real fresh game processes from fixed versions; preserve all fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import threading

p=argparse.ArgumentParser()
p.add_argument("--client",type=Path,required=True)
p.add_argument("--publisher",type=Path,required=True)
p.add_argument("--fixture",type=Path,required=True)
a=p.parse_args()
game,publisher,fixture=(x.resolve() for x in (a.client,a.publisher,a.fixture))
environment=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_RENDER_DRIVER="software",SDL_AUDIODRIVER="dummy")

def tree(root):
    return {str(f.relative_to(root)):hashlib.sha256(f.read_bytes()).hexdigest()
            for f in root.rglob("*") if f.is_file()}

def publish(source,root,name):
    result=subprocess.run([str(publisher),"--validate",str(source)],check=True,capture_output=True,text=True,timeout=10)
    receipt=root/(name+"-receipt.json");receipt.write_text(result.stdout)
    subprocess.run([str(publisher),"--publish",str(source),"--receipt",str(receipt),
        "--library",str(root/"versions"),"--revision",name],check=True,capture_output=True,text=True,timeout=10)
    return root/"versions"/name/"manifest.json"

def launch(root,manifest,revision):
    child=subprocess.Popen([str(game),"--world-version",str(manifest),"--no-save","--profile","fresh"],
        cwd=root,env=environment,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    ready=threading.Event();lines=[]
    def reader():
        for line in child.stdout:
            lines.append(line)
            if "Player start: fresh player" in line:
                ready.set();return
    thread=threading.Thread(target=reader,daemon=True);thread.start()
    try:
        if not ready.wait(10):
            child.kill();child.wait(timeout=10);thread.join(timeout=1)
            raise AssertionError("Published game did not initialize a fresh player: "+"".join(lines)+child.stderr.read())
        assert any("Published world: "+revision+" | SHA-256:" in line for line in lines),lines
        assert child.poll() is None,"Game exited before normal startup"
    finally:
        if child.poll() is None:child.terminate()
        child.wait(timeout=10);thread.join(timeout=1)
        child.stdout.close();child.stderr.close()

with tempfile.TemporaryDirectory(prefix="embervale-published-game-") as directory:
    root=Path(directory)
    source=root/"draft.json";source.write_bytes(fixture.read_bytes())
    first=publish(source,root,"v1")
    data=json.loads(source.read_bytes())
    spawn=data["player_spawn"];x,y=spawn["x"],spawn["y"]
    occupied={(p["position"]["x"],p["position"]["y"]) for p in data["placements"]}
    from_path=Path(__file__).resolve().parents[1]/"tools"
    import sys
    sys.path.insert(0,str(from_path))
    from embervale_world_editor import building_walls
    occupied |= building_walls(data.get("buildings",[]))
    for nx,ny in ((x+1,y),(x-1,y),(x,y+1),(x,y-1)):
        if 0<=nx<100 and 0<=ny<100 and data["terrain"]["rows"][ny][nx]!="W" and (nx,ny) not in occupied:
            data["player_spawn"]["x"],data["player_spawn"]["y"]=nx,ny
            break
    else:raise AssertionError("Fixture has no adjacent safe spawn for a second version")
    source.write_text(json.dumps(data))
    second=publish(source,root,"v2")
    (root/"saves").mkdir()
    for suffix in ("",".tmp",".bak"):
        (root/"saves"/("development-player.save"+suffix)).write_text("private profile must remain untouched")
    before=tree(root)
    for manifest,name in ((first,"v1"),(second,"v2"),(first,"v1")):
        launch(root,manifest,name)
        assert tree(root)==before,"Version launch changed a profile or published fixture"

    # Invalid/tampered version failures must precede both graphics and player loading.
    world=second.parent/"world.json"
    world.write_bytes(world.read_bytes()+b"\n")
    before=tree(root)
    result=subprocess.run([str(game),"--world-version",str(second),
        "--save",str(root/"saves/development-player.save")],cwd=root,env=environment,
        capture_output=True,text=True,timeout=10)
    assert result.returncode==1 and "SHA-256 mismatch" in result.stderr,result.stderr
    assert tree(root)==before,"Bad version startup modified player/content data"
    for target in (first,first.parent/"world.json",root/"versions/other/player.save"):
        result=subprocess.run([str(game),"--world-version",str(first),"--save",str(target)],
            cwd=root,env=environment,capture_output=True,text=True,timeout=10)
        assert result.returncode==2 and "outside the published version library" in result.stderr,result.stderr
        assert tree(root)==before,"Published-library save alias wrote files"
print("PASS: real v1/v2/v1 game startups, tamper rejection and library/save isolation")
