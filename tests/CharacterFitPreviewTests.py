"""Exercise the layered character through the real world renderer in an isolated directory."""
from pathlib import Path
from hashlib import sha256
import subprocess
import sys
import tempfile


def check(preview: Path, root: Path) -> None:
    world = root / "content/worlds/river-crossing.blockout.json"
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp)

        def render(name, *options, focus=("15", "25"), cwd=root, success=True):
            path = output / (name + ".png")
            cmd = [preview, world, path, "--focus", *focus, *options]
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

        # A shield takes precedence over the layered fixture in either direction.
        shield = ("--shield",)
        if render("shield", *shield) != render("shield-fit", *shield, *full):
            raise AssertionError("Custom layers obscured the existing equipped player")
        def step(name, x, target_x, outfit, frames=18):
            sequence = output / name
            render(name, *outfit, "--walk-to", target_x, "25", "--frames", str(frames),
                   "--frames-to", sequence, focus=(x, "25"))
            images = [sha256(path.read_bytes()).digest() for path in sorted(sequence.glob("frame-*.png"))]
            if len(images) != frames:
                raise AssertionError(f"{name} produced {len(images)} of {frames} frames")
            return images

        east = step("east-current", "15", "16", (), 31)
        east_full = step("east-full", "15", "16", full, 31)
        east_knee = step("east-knee", "15", "16", ("--fit-outfit", "knee"), 31)
        if any(len({a, b, c}) != 3 for a, b, c in zip(east, east_full, east_knee)):
            raise AssertionError("East route lost a fitted outfit during the turn, walk or arrival")
        if any(len(set(route[16:])) != 1 for route in (east_full, east_knee)):
            raise AssertionError("East standing pose did not hold after arrival")
        if step("east-shield", "15", "16", shield) != step("east-shield-fit", "15", "16", shield + full):
            raise AssertionError("East clothing obscured the equipped player")
        west = step("west-current", "16", "15", ())
        west_fit = step("west-fit", "16", "15", full)
        # Frame zero faces south. Subsequent frames move or stand facing west.
        if west[0] == west_fit[0] or west[1:] != west_fit[1:]:
            raise AssertionError("West fallback or the initial south outfit changed")

        render("missing-layers", *full, cwd=output, success=False)
        render("missing-outfit", "--fit-scale", "world", success=False)


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
