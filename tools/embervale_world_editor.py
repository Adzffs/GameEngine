"""First visual Embervale starter-world editor.

All edits and saves are executed by EmbervaleWorldMove's validated --session
command. This Python UI only renders a read-only terrain snapshot and replays
bounded user commands; it never serializes gameplay content to disk.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

SOURCE_HASH = re.compile(r"^Source SHA-256: ([0-9a-f]{64})$", re.MULTILINE)
PLACEMENT = re.compile(r"^([RSMN])  ([A-Za-z0-9._-]+)  \((\d+), (\d+)\)", re.MULTILINE)
KINDS = {"R": "Resource", "S": "Station", "N": "NPC spawn", "M": "Monster spawn"}
MAX_COMMANDS = 256
MIN_TILE = 4
MAX_TILE = 64


def tile_at(canvas_x: float, canvas_y: float, tile_size: int,
            width: int, height: int) -> tuple[int, int] | None:
    """Convert scroll-adjusted canvas pixels into an authored tile coordinate."""
    if tile_size <= 0:
        raise ValueError("tile size must be positive")
    x, y = int(canvas_x // tile_size), int(canvas_y // tile_size)
    return (x, y) if 0 <= x < width and 0 <= y < height else None


def scroll_fraction(world_pixels: int, viewport_pixels: int,
                    requested_origin: float) -> float:
    """Clamp viewport origin before converting it to Tk's fractional scroll."""
    if world_pixels <= 0:
        return 0.0
    origin = max(0.0, min(float(max(0, world_pixels - viewport_pixels)),
                          requested_origin))
    return origin / world_pixels


def fitted_tile(viewport_width: int, viewport_height: int,
                world_width: int, world_height: int) -> int:
    """Choose a pixel scale that exposes as much of the map as possible."""
    if world_width <= 0 or world_height <= 0:
        raise ValueError("world dimensions must be positive")
    return max(MIN_TILE, min(MAX_TILE, viewport_width // world_width,
                             viewport_height // world_height))


class StudioError(RuntimeError):
    """A user-visible validation or tool error."""



def verify_editor_transition(command: str, old_placements: dict, old_terrain: list[str],
                             old_status: str, new_placements: dict,
                             new_terrain: list[str], new_status: str) -> None:
    """Fail closed when the C++ response contradicts a requested editor action."""
    parts = command.split()
    action = parts[0] if parts else ""
    if action not in ("move", "paint", "undo", "redo"):
        return
    try:
        _, old_undo, old_redo = old_status.split("/")
        _, new_undo, new_redo = new_status.split("/")
        before, pending = int(old_undo), int(old_redo)
        after, remaining = int(new_undo), int(new_redo)
        if action in ("move", "paint"):
            history_ok = after == before + 1 and remaining == 0
        elif action == "undo":
            history_ok = after == before - 1 and remaining == pending + 1
        else:
            history_ok = after == before + 1 and remaining == pending - 1
        if not history_ok:
            raise StudioError("Editor returned inconsistent undo/redo history")
        if action == "move":
            name, x, y = parts[1], int(parts[2]), int(parts[3])
            expected = dict(old_placements)
            expected[name] = (expected[name][0], x, y)
            if new_placements != expected or new_terrain != old_terrain:
                raise StudioError("Editor returned an unexpected move or ground change")
        elif action == "paint":
            x, y, glyph = int(parts[1]), int(parts[2]), parts[3]
            expected = list(old_terrain)
            expected[y] = expected[y][:x] + glyph + expected[y][x + 1:]
            if new_placements != old_placements or new_terrain != expected:
                raise StudioError("Editor returned an unexpected paint or placement change")
        elif new_placements == old_placements and new_terrain == old_terrain:
            raise StudioError("Editor reported undo/redo without changing the world")
    except (IndexError, KeyError, TypeError, ValueError) as error:
        raise StudioError("Editor returned malformed command/history data") from error

def verify_editor_checkpoint(action: str, checkpoints: list[tuple[dict, tuple[str, ...]]],
                             placements: dict, terrain: list[str], status: str) -> list[tuple[dict, tuple[str, ...]]]:
    """Compare undo/redo with previously observed authoritative world revisions."""
    if action not in ("move", "paint", "undo", "redo"):
        return checkpoints
    try:
        cursor = int(status.split("/")[1])
    except (IndexError, ValueError, AttributeError) as error:
        raise StudioError("Editor returned a malformed history checkpoint") from error
    state = (dict(placements), tuple(terrain))
    if action in ("undo", "redo"):
        if cursor >= len(checkpoints) or cursor < 0 or state != checkpoints[cursor]:
            raise StudioError("Editor undo/redo disagrees with a verified history checkpoint")
        return checkpoints
    if cursor < 1 or cursor > len(checkpoints):
        raise StudioError("Editor returned an impossible edit history checkpoint")
    return [*checkpoints[:cursor], state]



def verify_saved_draft_receipt(transcript: str, destination: Path) -> None:
    """Accept only the requested new draft when the saved bytes match C++."""
    target = str(destination)
    acknowledgements = re.findall(r"^Saved new draft: (.+)$", transcript, re.MULTILINE)
    digests = re.findall(r"^draft-sha256: ([0-9a-f]{64})$", transcript, re.MULTILINE)
    if acknowledgements != [target] or len(digests) != 1:
        raise StudioError("Editor did not confirm the requested saved draft and hash")
    try:
        if destination.is_symlink() or not destination.is_file():
            raise StudioError("Editor did not publish a regular draft file")
        if destination.stat().st_size > 2 * 1024 * 1024:
            raise StudioError("Saved draft exceeds the 2 MiB input limit")
        saved_bytes = destination.read_bytes()
    except OSError as error:
        raise StudioError(f"Cannot verify saved draft: {error}") from error
    if hashlib.sha256(saved_bytes).hexdigest() != digests[0]:
        raise StudioError("Saved draft SHA-256 differs from the authoritative receipt")

class StudioBridge:
    """Revision-bound replay bridge to the *real* C++ editing session.

    Each action replays the bounded history against the original exact-byte
    revision. The C++ session validates every operation, including undo/redo;
    the UI commits its new action only if that replay succeeds. This avoids
    adding an independent JSON editing engine or a fragile interactive pipe.
    """

    def __init__(self, executable: Path, source: Path):
        self.executable = str(executable)
        if Path(source).is_symlink():
            raise StudioError("Select a regular starter-world snapshot file")
        self.source = Path(source).resolve()
        if not self.source.is_file():
            raise StudioError("Select a regular starter-world snapshot file")
        if self.source.stat().st_size > 2 * 1024 * 1024:
            raise StudioError("Snapshot exceeds the 2 MiB editor input limit")
        try:
            source_bytes = self.source.read_bytes()
            display_hash = hashlib.sha256(source_bytes).hexdigest()
            self.snapshot = json.loads(source_bytes.decode("utf-8"))
            terrain = self.snapshot["terrain"]["rows"]
            width, height = (self.snapshot["size"][k] for k in ("width", "height"))
            if not isinstance(width, int) or not isinstance(height, int) or not (0 < width <= 256 and 0 < height <= 256):
                raise ValueError("invalid dimensions")
            if len(terrain) != height or any(not isinstance(row, str) or len(row) != width for row in terrain):
                raise ValueError("invalid terrain shape")
            # Startup rows only cross-check the authoritative C++ session.
            display_terrain = list(terrain)
            self.terrain = list(terrain)
            self.width, self.height = width, height
            self.spawn = (self.snapshot["player_spawn"]["x"], self.snapshot["player_spawn"]["y"])
            self.definitions = {p["id"]: p["definition_id"] for p in self.snapshot["placements"]}
            kinds = {"resource": "R", "station": "S", "npc_spawn": "N",
                     "monster_spawn": "M"}
            self.source_placements = {
                p["id"]: (kinds[p["kind"]], p["position"]["x"], p["position"]["y"])
                for p in self.snapshot["placements"]
            }
            if len(self.source_placements) != len(self.snapshot["placements"]):
                raise ValueError("duplicate source placement IDs")
        except (OSError, UnicodeError, ValueError, TypeError, KeyError) as error:
            raise StudioError(f"Cannot display snapshot: {error}") from error
        self.hash: str | None = None
        self.commands: list[str] = []
        self.placements: dict[str, tuple[str, int, int]] = {}
        self.status = ""
        self._accept(self._invoke([]))
        if self.hash != display_hash:
            raise StudioError("CONTENT_CONFLICT: source changed while opening; reopen the editor")
        if self.placements != self.source_placements:
            raise StudioError("The authoritative placements differ from the source snapshot")
        if self.terrain != display_terrain:
            raise StudioError("The authoritative terrain differs from the source snapshot")
        # Keep only responses already observed from C++; never infer undo inverses.
        self._checkpoints = [(dict(self.placements), tuple(self.terrain))]

    def _invoke(self, commands: list[str]) -> tuple[str, dict[str, tuple[str, int, int]], str, list[str], str]:
        arguments = [self.executable, "--session", str(self.source)]
        if self.hash:
            arguments += ["--expected-hash", self.hash]
        script = "\n".join([*commands, "list", "terrain", "status", "quit", ""])
        try:
            result = subprocess.run(arguments, input=script, text=True,
                                    encoding="utf-8", errors="replace",
                                    capture_output=True, timeout=20, check=False)
        except (OSError, subprocess.TimeoutExpired) as error:
            raise StudioError(f"World editing tool failed: {error}") from error
        if result.returncode != 0 or result.stderr.strip():
            raise StudioError(result.stderr.strip() or f"Editor exited with code {result.returncode}")
        match = SOURCE_HASH.search(result.stdout)
        if not match:
            raise StudioError("Editor did not return its source revision")
        digest = match.group(1)
        if self.hash is not None and digest != self.hash:
            raise StudioError("CONTENT_CONFLICT: source changed; reopen the editor")
        marker = "Existing placements (only R and S can be moved):\n"
        if marker not in result.stdout:
            raise StudioError("Editor did not return a placement list")
        placements: dict[str, tuple[str, int, int]] = {}
        for kind, name, x, y in PLACEMENT.findall(result.stdout.split(marker)[-1]):
            if name in placements:
                raise StudioError("Duplicate placement returned by editor")
            placements[name] = (kind, int(x), int(y))
        if not placements:
            raise StudioError("Editor returned no placements")
        # This is the only terrain used by the viewport. Never locally guess
        # a painted tile or inverse from the command string: the authoritative
        # C++ history has already validated and replayed the complete world.
        pieces = result.stdout.rsplit("Terrain rows (GDSW):\n", 1)
        if len(pieces) != 2:
            raise StudioError("Editor did not return authoritative terrain")
        block, delimiter, _ = pieces[1].partition("End terrain rows\n")
        rows = block.splitlines()
        if (not delimiter or len(rows) != self.height or
                any(len(row) != self.width or not set(row) <= set("GDSW") for row in rows)):
            raise StudioError("Editor returned malformed authoritative terrain")
        statuses = re.findall(r"^Changed: (yes|no); undo: (\d+); redo: (\d+); original SHA-256: ([0-9a-f]{64})$",
                              result.stdout, re.MULTILINE)
        if not statuses or statuses[-1][-1] != digest:
            raise StudioError("Editor did not return an authoritative status")
        return digest, placements, "/".join(statuses[-1][:3]), rows, result.stdout

    def _accept(self, result: tuple[str, dict[str, tuple[str, int, int]], str, list[str], str]) -> None:
        self.hash, self.placements, self.status, self.terrain, _ = result

    def perform(self, command: str) -> None:
        if any(c in command for c in "\r\n"):
            raise StudioError("Newlines are not valid editor commands")
        if len(self.commands) >= MAX_COMMANDS:
            raise StudioError("This session reached its 256-command limit; save a draft and reopen it")
        result = self._invoke([*self.commands, command])
        if set(result[1]) != set(self.source_placements):
            raise StudioError("Editing changed the authoritative placement identities")
        for name, (kind, x, y) in result[1].items():
            original_kind, original_x, original_y = self.source_placements[name]
            if kind != original_kind:
                raise StudioError("Editing changed an authoritative placement kind")
            if not (0 <= x < self.width and 0 <= y < self.height):
                raise StudioError("Editor returned an out-of-bounds placement")
            if kind in ("N", "M") and (x, y) != (original_x, original_y):
                raise StudioError("Editor moved a read-only actor spawn")
        # Both displayed placements and painted ground come from the same
        # successful backend replay. Rejections preserve all local state.
        verify_editor_transition(command, self.placements, self.terrain, self.status,
                                 result[1], result[3], result[2])
        checkpoints = verify_editor_checkpoint(command.split()[0], self._checkpoints,
                                               result[1], result[3], result[2])
        self._accept(result)
        self._checkpoints = checkpoints
        self.commands.append(command)

    def move(self, placement_id: str, x: int, y: int) -> None:
        if not re.fullmatch(r"[A-Za-z0-9._-]+", placement_id):
            raise StudioError("Invalid placement ID")
        entry = self.placements.get(placement_id)
        if entry is None or entry[0] not in ("R", "S"):
            raise StudioError("Only existing resource and station placements are movable")
        if not (0 <= x < self.width and 0 <= y < self.height):
            raise StudioError("Destination is outside the starter world")
        if (x, y) != (entry[1], entry[2]):
            self.perform(f"move {placement_id} {x} {y}")

    def paint(self, x: int, y: int, glyph: str) -> None:
        if glyph not in ("G", "D", "S", "W"):
            raise StudioError("Invalid terrain brush")
        if not (0 <= x < self.width and 0 <= y < self.height):
            raise StudioError("Paint tile is outside the map")
        if self.terrain[y][x] != glyph:
            self.perform(f"paint {x} {y} {glyph}")

    def undo(self) -> None:
        self.perform("undo")

    def redo(self) -> None:
        self.perform("redo")

    def save(self, destination: Path) -> str:
        target = str(Path(destination).resolve())
        if "\n" in target or "\r" in target:
            raise StudioError("Invalid draft filename")
        # The C++ writer validates source SHA and publishes only a new sibling.
        result = self._invoke([*self.commands, "save " + target])
        verify_saved_draft_receipt(result[4], Path(target))
        return target


def launch(bridge: StudioBridge) -> None:
    # Imported lazily: headless tests and command-line help do not need a display.
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk

    root = tk.Tk()
    root.title("Embervale Studio | Starter World")
    root.geometry("1120x760")
    tile = 22
    selected: str | None = None
    brush = tk.StringVar(value="Select / move")
    saved: str | None = None
    caption = tk.StringVar()
    cursor_label = tk.StringVar(value="Tile: —")
    inspector = tk.StringVar(value="Click a resource or station, then click an empty target tile")
    frame = ttk.Frame(root, padding=8)
    frame.pack(fill="both", expand=True)
    toolbar = ttk.Frame(frame)
    toolbar.pack(fill="x")
    status_label = ttk.Label(toolbar, textvariable=caption)
    status_label.pack(side="left", padx=(0, 16))
    body = ttk.Frame(frame)
    body.pack(fill="both", expand=True, pady=(8, 0))
    sidebar = ttk.Frame(body, width=260)
    sidebar.pack(side="right", fill="y", padx=(8, 0))
    ttk.Label(sidebar, text="Placements (R/S movable)").pack(anchor="w")
    choices = tk.Listbox(sidebar, width=40, height=25, exportselection=False)
    choices.pack(fill="both", expand=True)
    ttk.Label(sidebar, textvariable=inspector, wraplength=250).pack(anchor="w", pady=8)
    viewport = ttk.Frame(body)
    viewport.pack(side="left", fill="both", expand=True)
    canvas = tk.Canvas(viewport, background="#222222", highlightthickness=0)
    horizontal = ttk.Scrollbar(viewport, orient="horizontal", command=canvas.xview)
    vertical = ttk.Scrollbar(viewport, orient="vertical", command=canvas.yview)
    canvas.configure(xscrollcommand=horizontal.set, yscrollcommand=vertical.set)
    canvas.grid(row=0, column=0, sticky="nsew")
    vertical.grid(row=0, column=1, sticky="ns")
    horizontal.grid(row=1, column=0, sticky="ew")
    viewport.columnconfigure(0, weight=1)
    viewport.rowconfigure(0, weight=1)
    terrain_colors = {"G": "#54783d", "D": "#8b7650", "S": "#7c8089", "W": "#3978ad"}
    object_colors = {"R": "#e1d34b", "S": "#e89c4b", "N": "#f6f4f0", "M": "#e55d70"}

    def display() -> None:
        canvas.delete("all")
        canvas.configure(scrollregion=(0, 0, tile * bridge.width, tile * bridge.height))
        for y, row in enumerate(bridge.terrain):
            for x, symbol in enumerate(row):
                canvas.create_rectangle(x * tile, y * tile, (x + 1) * tile,
                                        (y + 1) * tile, fill=terrain_colors.get(symbol, "#222222"),
                                        outline="#354235", width=1)
        spawn_x, spawn_y = bridge.spawn
        if tile >= 12:
            canvas.create_text((spawn_x + .5) * tile, (spawn_y + .5) * tile,
                               text="@", fill="white",
                               font=("TkDefaultFont", max(9, tile - 6), "bold"))
        else:
            canvas.create_rectangle(spawn_x * tile, spawn_y * tile,
                                    (spawn_x + 1) * tile, (spawn_y + 1) * tile,
                                    fill="white", outline="#303030")
        marker_inset = max(1, min(2, tile // 6))
        for name, (kind, x, y) in bridge.placements.items():
            canvas.create_oval(x * tile + marker_inset, y * tile + marker_inset,
                               (x + 1) * tile - marker_inset,
                               (y + 1) * tile - marker_inset,
                               fill=object_colors[kind],
                               outline="white" if name == selected else "#202020",
                               width=3 if name == selected else 1)
            if name == selected:
                inspector.set(f"{KINDS[kind]}\n{name}\n{bridge.definitions[name]}\nTile: ({x}, {y})")
        caption.set(f"{bridge.source.name} | {bridge.hash[:10]} | {tile}px/tile | changed/undo/redo: {bridge.status}")
        choices.delete(0, "end")
        names = sorted(bridge.placements)
        for name in names:
            kind, x, y = bridge.placements[name]
            choices.insert("end", f"{kind} {name} ({x},{y})")
        if selected in names:
            choices.selection_set(names.index(selected))

    def zoom_to(requested_tile: int, anchor: tuple[int, int] | None = None) -> None:
        nonlocal tile
        next_tile = max(MIN_TILE, min(MAX_TILE, requested_tile))
        if next_tile == tile:
            return
        if anchor is None:
            anchor = (canvas.winfo_width() // 2, canvas.winfo_height() // 2)
        # Preserve the same world point beneath the cursor / viewport centre.
        world_x = canvas.canvasx(anchor[0]) / tile
        world_y = canvas.canvasy(anchor[1]) / tile
        tile = next_tile
        display()
        canvas.xview_moveto(scroll_fraction(
            bridge.width * tile, canvas.winfo_width(), world_x * tile - anchor[0]))
        canvas.yview_moveto(scroll_fraction(
            bridge.height * tile, canvas.winfo_height(), world_y * tile - anchor[1]))

    def fit_view() -> None:
        nonlocal tile
        root.update_idletasks()
        tile = fitted_tile(canvas.winfo_width(), canvas.winfo_height(),
                           bridge.width, bridge.height)
        display()
        canvas.xview_moveto(0)
        canvas.yview_moveto(0)

    def select(name: str) -> None:
        nonlocal selected
        selected = name
        display()
        kind, x, y = bridge.placements[name]
        canvas.xview_moveto(max(0, x * tile - 260) / max(1, bridge.width * tile))
        canvas.yview_moveto(max(0, y * tile - 220) / max(1, bridge.height * tile))

    def on_choose(_event: object) -> None:
        indexes = choices.curselection()
        if indexes:
            name = sorted(bridge.placements)[indexes[0]]
            if name != selected:
                select(name)

    def on_click(event: tk.Event) -> None:
        position = tile_at(canvas.canvasx(event.x), canvas.canvasy(event.y),
                           tile, bridge.width, bridge.height)
        if position is None:
            return
        x, y = position
        if brush.get() != "Select / move":
            try:
                bridge.paint(x, y, {"Grass": "G", "Dirt": "D",
                                    "Stone": "S", "Water": "W"}[brush.get()])
                display()
            except StudioError as error:
                messagebox.showerror("Paint rejected", str(error), parent=root)
            return
        occupant = next((name for name, (_, px, py) in bridge.placements.items()
                         if (x, y) == (px, py)), None)
        if occupant:
            select(occupant)
        elif selected:
            try:
                bridge.move(selected, x, y)
                display()
            except StudioError as error:
                messagebox.showerror("Move rejected", str(error), parent=root)

    def on_hover(event: tk.Event) -> None:
        position = tile_at(canvas.canvasx(event.x), canvas.canvasy(event.y),
                           tile, bridge.width, bridge.height)
        cursor_label.set(f"Tile: {position[0]}, {position[1]}" if position else "Tile: —")

    def zoom_wheel(event: tk.Event, direction: int) -> str:
        zoom_to(tile + direction * 2, (event.x, event.y))
        return "break"

    def action(name: str) -> None:
        try:
            bridge.perform(name)
            display()
        except StudioError as error:
            messagebox.showerror("Edit rejected", str(error), parent=root)

    def save() -> None:
        nonlocal saved
        path = filedialog.asksaveasfilename(parent=root, title="Save new draft (never overwrites)",
                                            initialdir=str(bridge.source.parent),
                                            defaultextension=".json", filetypes=[("JSON", "*.json")])
        if not path:
            return
        try:
            saved = bridge.save(Path(path))
            messagebox.showinfo("Draft saved", f"Created {saved}\nOriginal world and player saves were not modified.", parent=root)
        except StudioError as error:
            messagebox.showerror("Save rejected", str(error), parent=root)

    ttk.Label(toolbar, text="Tool:").pack(side="left", padx=(4, 2))
    ttk.Combobox(toolbar, textvariable=brush, state="readonly", width=15,
                 values=("Select / move", "Grass", "Dirt", "Stone", "Water")).pack(side="left", padx=(0, 8))
    ttk.Button(toolbar, text="−", command=lambda: zoom_to(tile - 2)).pack(side="left", padx=2)
    ttk.Button(toolbar, text="+", command=lambda: zoom_to(tile + 2)).pack(side="left", padx=2)
    ttk.Button(toolbar, text="Fit", command=fit_view).pack(side="left", padx=(2, 8))
    ttk.Label(toolbar, textvariable=cursor_label).pack(side="right", padx=5)
    ttk.Button(toolbar, text="Undo (Ctrl+Z)", command=lambda: action("undo")).pack(side="left", padx=3)
    ttk.Button(toolbar, text="Redo (Ctrl+Y)", command=lambda: action("redo")).pack(side="left", padx=3)
    ttk.Button(toolbar, text="Save new draft", command=save).pack(side="left", padx=3)
    ttk.Label(frame, text="Grass green • dirt brown • stone grey • water blue • resources yellow • stations orange • NPC white • monsters pink • @ player start").pack(anchor="w", pady=(5, 0))
    root.bind_all("<Control-z>", lambda _event: action("undo"))
    root.bind_all("<Control-y>", lambda _event: action("redo"))
    canvas.bind("<Button-1>", on_click)
    canvas.bind("<Motion>", on_hover)
    canvas.bind("<Control-MouseWheel>", lambda event: zoom_wheel(event, 1 if event.delta > 0 else -1))
    canvas.bind("<Control-Button-4>", lambda event: zoom_wheel(event, 1))
    canvas.bind("<Control-Button-5>", lambda event: zoom_wheel(event, -1))
    root.bind_all("<Control-equal>", lambda _event: zoom_to(tile + 2))
    root.bind_all("<Control-plus>", lambda _event: zoom_to(tile + 2))
    root.bind_all("<Control-minus>", lambda _event: zoom_to(tile - 2))
    root.bind_all("<Control-0>", lambda _event: fit_view())
    choices.bind("<<ListboxSelect>>", on_choose)
    display()
    root.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser(description="Visual starter-world editor backed by EmbervaleWorldMove")
    parser.add_argument("snapshot", type=Path, help="Existing validated starter-world snapshot JSON")
    parser.add_argument("--tool", type=Path, default=Path("EmbervaleWorldMove"),
                        help="Path to the built EmbervaleWorldMove executable")
    args = parser.parse_args()
    try:
        launch(StudioBridge(args.tool, args.snapshot))
    except StudioError as error:
        parser.exit(1, f"Embervale Studio: {error}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
