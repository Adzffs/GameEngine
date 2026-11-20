"""Kill a live process after a periodic checkpoint, then reload in a new process."""
import pathlib
import subprocess
import sys
import tempfile
import threading

executable = sys.argv[1]
with tempfile.TemporaryDirectory(prefix="embervale-autosave-interruption-") as temporary:
    save = pathlib.Path(temporary) / "profile" / "player.save"
    child = subprocess.Popen(
        [executable, "--checkpoint-and-wait", str(save)],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    ready = threading.Event()
    lines = []

    def read_ready():
        for line in child.stdout:
            lines.append(line)
            if line.strip() == "checkpoint ready":
                ready.set()
                return

    reader = threading.Thread(target=read_ready, daemon=True)
    reader.start()
    try:
        if not ready.wait(10):
            raise AssertionError("Child did not publish the periodic checkpoint: " + "".join(lines))
        child.kill()
        child.wait(timeout=10)
        assert child.returncode != 0, "Child must terminate without controlled shutdown"
        subprocess.run([executable, "--verify-checkpoint", str(save)], check=True, timeout=10)
        assert not pathlib.Path(str(save) + ".tmp").exists(), "Checkpoint left a temp sidecar"
        assert not pathlib.Path(str(save) + ".bak").exists(), "Checkpoint left a backup sidecar"
    finally:
        if child.poll() is None:
            child.kill()
        child.communicate(timeout=10)
        reader.join(timeout=1)
print("PASS: killed process reloads 71 saved coins; its later unsaved coin is absent")
