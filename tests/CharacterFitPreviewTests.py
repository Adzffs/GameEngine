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

        shield = ("--shield",)
        if len({render("shield", *shield), render("shield-fit", *shield, *full),
                render("shield-knee", *shield, "--fit-outfit", "knee"), fitted}) != 4:
            raise AssertionError("Equipping the shield lost the shield, body or trouser choice")
        def walk(name, start, end, outfit, frames=18, via=None):
            sequence = output / name
            waypoint = ("--walk-via", *map(str, via)) if via else ()
            render(name, *outfit, "--walk-to", *map(str, end), *waypoint, "--frames", str(frames),
                   "--frames-to", sequence, focus=tuple(map(str, start)))
            images = [sha256(path.read_bytes()).digest() for path in sorted(sequence.glob("frame-*.png"))]
            if len(images) != frames:
                raise AssertionError(f"{name} produced {len(images)} of {frames} frames")
            return images

        start, corner = (15, 25), (16, 25)
        short = ("--fit-outfit", "knee")
        east = walk("east-current", start, corner, (), 31)
        east_full = walk("east-full", start, corner, full, 31)
        east_knee = walk("east-knee", start, corner, short, 31)
        if any(len({a, b, c}) != 3 for a, b, c in zip(east, east_full, east_knee)):
            raise AssertionError("East route lost a fitted outfit during the turn, walk or arrival")
        if any(len(set(route[16:])) != 1 for route in (east_full, east_knee)):
            raise AssertionError("East standing pose did not hold after arrival")
        for direction, end in (("north", (16, 24)), ("west", start)):
            turn = walk(direction + "-current", start, end, (), 55, via=corner)
            turn_full = walk(direction + "-full", start, end, full, 55, via=corner)
            turn_knee = walk(direction + "-knee", start, end, short, 55, via=corner)
            for straight, turning in ((east, turn), (east_full, turn_full), (east_knee, turn_knee)):
                if straight[:16] != turning[:16] or straight[16] == turning[16]:
                    raise AssertionError(f"The {direction} turn did not begin immediately after the east waypoint")
            if any(len({a, b, c}) != 3 for a, b, c in zip(turn[16:], turn_full[16:], turn_knee[16:])):
                raise AssertionError(f"The {direction} leg lost a fitted outfit during movement or arrival")
            # Let the camera finish following the player after the last tile.
            if any(len(set(route[-12:])) != 1 for route in (turn_full, turn_knee)):
                raise AssertionError(f"The {direction} standing pose did not hold after arrival")
            equipped = walk(direction + "-shield", start, end, shield, 33, via=corner)
            fitted_equipped = walk(direction + "-shield-fit", start, end, shield + full, 33, via=corner)
            if any(a == b for a, b in zip(equipped, fitted_equipped)):
                raise AssertionError(f"The {direction} shield walk reverted to the old player")
        # Approach the same junction from each corner. Once the camera settles
        # the same world is visible, so each held diagonal must still be distinct.
        junction = (17, 27)
        diagonal_stands = set()
        for facing, origin in (("south-east", (16, 26)), ("south-west", (18, 26)),
                               ("north-east", (16, 28)), ("north-west", (18, 28))):
            ordinary = walk(facing + "-current", origin, junction, (), 43)
            trousers = walk(facing + "-full", origin, junction, full, 43)
            shorts = walk(facing + "-knee", origin, junction, short, 43)
            if any(len({a, b, c}) != 3 for a, b, c in zip(ordinary, trousers, shorts)):
                raise AssertionError(f"{facing} lost its layered outfit or trouser choice")
            if any(len(set(route[-6:])) != 1 for route in (trousers, shorts)):
                raise AssertionError(f"{facing} did not hold its standing pose")
            diagonal_stands.update((trousers[-1], shorts[-1]))
        if len(diagonal_stands) != 8:
            raise AssertionError("Two fitted diagonal stands rendered the same image")
        armed = ("--weapon", "bronze-sword", "--shield")
        old = walk("diagonal-armed", (16, 26), junction, armed)
        new = walk("diagonal-armed-fit", (16, 26), junction, armed + full)
        if any(a == b for a, b in zip(old, new)):
            raise AssertionError("The diagonal equipped walk reverted to the old player")

        # Exercise active World actions through Graphics, including shield + tool.
        for action, focus, target in (("chop", ("7","55"), ("8","55")), ("mine", ("83","60"), ("84","60")),
                                      ("melee", ("26","68"), ("27","68"))):
            options = ("--"+action, *target, "--shield", "--frames", "25")
            histories = []
            for variant, outfit in (("existing", ()), ("full", full), ("knee", short)):
                name = action + "-" + variant
                render(name, *options, *outfit, "--frames-to", output/name, focus=focus)
                histories.append([sha256(p.read_bytes()).digest() for p in sorted((output/name).glob("frame-*.png"))])
            if any(len(frames) != 25 for frames in histories) or any(len(set(row)) != 3 for row in zip(*histories)):
                raise AssertionError(f"{action} lost the fitted outfit during its action")

        render("missing-layers", *full, cwd=output, success=False)
        render("missing-outfit", "--fit-scale", "world", success=False)
        render("waypoint-without-walk", "--walk-via", "16", "25", "--frames", "12",
               "--frames-to", output / "no-route", success=False)
        render("waypoint-without-sequence", "--walk-via", "16", "25", "--walk-to", "16", "24", success=False)
        render("invalid-waypoint", "--walk-to", "16", "24", "--walk-via", "-1", "25", "--frames", "12",
               "--frames-to", output / "invalid-route", success=False)


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve())
