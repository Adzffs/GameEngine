"""Exercise the layered character through the real world renderer in an isolated directory."""
from pathlib import Path
import subprocess
import sys
import tempfile


def check(preview: Path, root: Path) -> None:
    world = root / "content/worlds/river-crossing.blockout.json"
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp)

        def render(name, *options, cwd=root, success=True):
            path = output / (name + ".png")
            cmd = [preview, world, path, "--focus", "15", "25", *options]
            result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=30)
            if (result.returncode == 0) != success:
                raise AssertionError(f"{name} returned {result.returncode}\n{result.stdout}\n{result.stderr}")
            if success:
                return path.read_bytes()
            if path.exists():
                raise AssertionError(f"Failed {name} capture produced a partial image")

        full = ("--fit-outfit", "full")
        current = render("current")
        fitted = render("fitted", *full)
        knee = render("knee", "--fit-outfit", "knee")
        native = render("native", *full, "--fit-scale", "native")
        if len({current, fitted, knee, native}) != 4:
            raise AssertionError("World comparison lost a distinct outfit or scale")

        # Equip a shield while facing south, then inspect a real eastward step.
        shield = ("--shield",)
        if render("shield", *shield) != render("shield-fit", *shield, *full):
            raise AssertionError("Custom layers obscured the existing equipped player")
        route = ("--walk-to", "16", "25", "--frames", "18", "--frames-to")
        for name, outfit in (("default-route", ()), ("fit-route", full)):
            render(name, *outfit, *route, output / name)
        for frame in range(1, 18):
            filename = f"frame-{frame:03d}.png"
            if (output / "default-route" / filename).read_bytes() != (output / "fit-route" / filename).read_bytes():
                raise AssertionError(f"Non-south fallback differs at frame {frame}")

        render("missing-layers", *full, cwd=output, success=False)
        render("missing-outfit", "--fit-scale", "world", success=False)


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
