"""First visual Embervale starter-world editor.

All edits and saves are executed by EmbervaleWorldMove's validated --session
command. This Python UI only renders a read-only terrain snapshot and replays
bounded user commands; it never serializes gameplay content to disk.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path

SOURCE_HASH = re.compile(r"^Source SHA-256: ([0-9a-f]{64})$", re.MULTILINE)
PLACEMENT = re.compile(r"^([RSMN])  ([A-Za-z0-9._-]+)  \((\d+), (\d+)\)", re.MULTILINE)
KINDS = {"R": "Resource", "S": "Station", "N": "NPC spawn", "M": "Monster spawn"}
MAX_COMMANDS = 256


class StudioError(RuntimeError):
    """A user-visible validation or tool error."""


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
            self.snapshot = json.loads(self.source.read_text(encoding="utf-8"))
            terrain = self.snapshot["terrain"]["rows"]
            width, height = (self.snapshot["size"][k] for k in ("width", "height"))
            if not isinstance(width, int) or not isinstance(height, int) or not (0 < width <= 256 and 0 < height <= 256):
                raise ValueError("invalid dimensions")
            if len(terrain) != height or any(not isinstance(row, str) or len(row) != width for row in terrain):
                raise ValueError("invalid terrain shape")
            self.terrain = terrain
            self.width, self.height = width, height
            self.spawn = (self.snapshot["player_spawn"]["x"], self.snapshot["player_spawn"]["y"])
            self.definitions = {p["id"]: p["definition_id"] for p in self.snapshot["placements"]}
        except (OSError, UnicodeError, ValueError, TypeError, KeyError) as error:
            raise StudioError(f"Cannot display snapshot: {error}") from error
        self.hash: str | None = None
        self.commands: list[str] = []
        self.placements: dict[str, tuple[str, int, int]] = {}
        self.status = ""
        self._accept(self._invoke([]))
        if set(self.definitions) != set(self.placements):
            raise StudioError("The authoritative placement list differs from the snapshot")

    def _invoke(self, commands: list[str]) -> tuple[str, dict[str, tuple[str, int, int]], str, str]:
        arguments = [self.executable, "--session", str(self.source)]
        if self.hash:
            arguments += ["--expected-hash", self.hash]
        script = "\n".join([*commands, "list", "status", "quit", ""])
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
        statuses = re.findall(r"^Changed: (yes|no); undo: (\d+); redo: (\d+); original SHA-256: ([0-9a-f]{64})$",
                              result.stdout, re.MULTILINE)
        if not statuses or statuses[-1][-1] != digest:
            raise StudioError("Editor did not return an authoritative status")
        return digest, placements, "/".join(statuses[-1][:3]), result.stdout

    def _accept(self, result: tuple[str, dict[str, tuple[str, int, int]], str, str]) -> None:
        self.hash, self.placements, self.status, _ = result

    def perform(self, command: str) -> None:
        if any(c in command for c in "\r\n"):
            raise StudioError("Newlines are not valid editor commands")
        if len(self.commands) >= MAX_COMMANDS:
            raise StudioError("This session reached its 256-command limit; save a draft and reopen it")
        result = self._invoke([*self.commands, command])
        if set(result[1]) != set(self.definitions):
            raise StudioError("Editing changed the authoritative placement identities")
        self.commands.append(command)
        self._accept(result)

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
        if "Saved new draft: " not in result[3]:
            raise StudioError("Editor did not confirm publication")
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
    saved: str | None = None
    caption = tk.StringVar()
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
        canvas.create_text((spawn_x + .5) * tile, (spawn_y + .5) * tile,
                           text="@", fill="white", font=("TkDefaultFont", max(9, tile - 6), "bold"))
        for name, (kind, x, y) in bridge.placements.items():
            canvas.create_oval(x * tile + 2, y * tile + 2, (x + 1) * tile - 2,
                               (y + 1) * tile - 2, fill=object_colors[kind],
                               outline="white" if name == selected else "#202020",
                               width=3 if name == selected else 1)
            if name == selected:
                inspector.set(f"{KINDS[kind]}\n{name}\n{bridge.definitions[name]}\nTile: ({x}, {y})")
        caption.set(f"{bridge.source.name} | {bridge.hash[:10]} | changed/undo/redo: {bridge.status}")
        choices.delete(0, "end")
        names = sorted(bridge.placements)
        for name in names:
            kind, x, y = bridge.placements[name]
            choices.insert("end", f"{kind} {name} ({x},{y})")
        if selected in names:
            choices.selection_set(names.index(selected))

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
        x, y = int(canvas.canvasx(event.x) // tile), int(canvas.canvasy(event.y) // tile)
        if not (0 <= x < bridge.width and 0 <= y < bridge.height):
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

    ttk.Button(toolbar, text="Undo (Ctrl+Z)", command=lambda: action("undo")).pack(side="left", padx=3)
    ttk.Button(toolbar, text="Redo (Ctrl+Y)", command=lambda: action("redo")).pack(side="left", padx=3)
    ttk.Button(toolbar, text="Save new draft", command=save).pack(side="left", padx=3)
    ttk.Label(frame, text="Grass green • dirt brown • stone grey • water blue • resources yellow • stations orange • NPC white • monsters pink • @ player start").pack(anchor="w", pady=(5, 0))
    root.bind_all("<Control-z>", lambda _event: action("undo"))
    root.bind_all("<Control-y>", lambda _event: action("redo"))
    canvas.bind("<Button-1>", on_click)
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
