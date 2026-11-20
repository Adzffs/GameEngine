"""Studio's explicit validate/publish/select-version flow."""
from pathlib import Path
import re

def show_versions(parent, bridge, client, library: Path, play):
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
    from embervale_publication import PublicationError
    from embervale_recovery import RecoveryError
    from embervale_world_editor import StudioError

    window = tk.Toplevel(parent)
    window.title("World versions | Embervale Studio")
    window.geometry("780x650")
    window.minsize(720, 620)
    window.transient(parent)
    frame = ttk.Frame(window, padding=16)
    frame.pack(fill="both", expand=True)
    ttk.Label(frame, text="Publish and play world versions", font=("", 14, "bold")).pack(anchor="w")
    ttk.Label(frame, text="Validate a saved draft, publish a fixed version, then select the version to play.",
              wraplength=730).pack(anchor="w", pady=(6, 12))
    library_text = tk.StringVar(value=str(library))
    row = ttk.Frame(frame)
    row.pack(fill="x")
    ttk.Label(row, text="Version library:").pack(side="left")
    ttk.Entry(row, textvariable=library_text).pack(side="left", fill="x", expand=True, padx=8)
    def browse():
        chosen = filedialog.askdirectory(parent=window, title="Choose version library", mustexist=False)
        if chosen:
            library_text.set(chosen)
            refresh()
    ttk.Button(row, text="Browse…", command=browse).pack(side="right")

    draft_box = ttk.LabelFrame(frame, text=f"Draft: {bridge.source.name}", padding=10)
    draft_box.pack(fill="x", pady=12)
    name = re.sub("[^a-z0-9-]", "-", bridge.source.stem.lower()).strip("-")[:42].rstrip("-") or "world"
    revision_text = tk.StringVar(value=name + "-v1")
    draft_status = tk.StringVar(value="Save a new draft first." if bridge.dirty else "Ready to validate.")
    ttk.Label(draft_box, textvariable=draft_status, wraplength=700).pack(anchor="w")
    row = ttk.Frame(draft_box)
    row.pack(fill="x", pady=(8, 0))
    ttk.Label(row, text="Version name:").pack(side="left")
    ttk.Entry(row, textvariable=revision_text, width=30).pack(side="left", padx=8)

    selected = None
    paths = []
    version_status = tk.StringVar(value="Select a published version to verify it.")
    def error(title, failure):
        messagebox.showerror(title, str(failure), parent=window)

    def validate():
        try:
            client.validate(bridge)
            draft_status.set("Draft validated, including required placements and access routes.")
            publish_button.configure(state="normal")
        except (OSError, PublicationError, RecoveryError, StudioError) as failure:
            publish_button.configure(state="disabled")
            draft_status.set("Validation did not complete; the draft is unchanged.")
            error("Validation failed", failure)

    def publish():
        try:
            if not library_text.get().strip():
                raise PublicationError("Choose a version library directory")
            version = client.publish(bridge, Path(library_text.get()), revision_text.get())
            draft_status.set(f"Published {version.metadata['revision']}. Existing versions were kept.")
            refresh(version.manifest)
        except (OSError, PublicationError, RecoveryError, StudioError) as failure:
            publish_button.configure(state="disabled")
            draft_status.set("Publishing did not complete. Validate again before retrying.")
            error("Publishing failed", failure)

    ttk.Button(row, text="Validate draft", command=validate,
               state="disabled" if bridge.dirty else "normal").pack(side="left")
    publish_button = ttk.Button(row, text="Publish version", command=publish, state="disabled")
    publish_button.pack(side="right")

    ttk.Label(frame, text="Published versions").pack(anchor="w")
    choices = tk.Listbox(frame, exportselection=False, height=8)
    choices.pack(fill="both", expand=True, pady=8)
    ttk.Label(frame, textvariable=version_status, wraplength=730).pack(anchor="w")
    ttk.Label(frame, text="To roll back the next launch, select an earlier version and press Play selected version.",
              wraplength=730).pack(anchor="w", pady=(10, 0))
    ttk.Label(frame, text="Player progress is temporary for Studio launches.", wraplength=730).pack(anchor="w")
    footer = ttk.Frame(frame)
    footer.pack(fill="x", pady=(12, 0))

    def inspect(_event=None):
        nonlocal selected
        selected = None
        play_button.configure(state="disabled")
        picked = choices.curselection()
        if not picked:
            return
        try:
            selected = client.inspect(paths[picked[0]])
            version_status.set(f"{selected.metadata['revision']} is ready to play. Map fingerprint: "
                               f"{selected.metadata['world_sha256'][:12]}")
            play_button.configure(state="normal")
        except (OSError, PublicationError, RecoveryError, StudioError) as failure:
            version_status.set(f"This version cannot be loaded: {failure}")

    def refresh(preferred=None):
        nonlocal paths, selected
        selected = None
        choices.delete(0, "end")
        play_button.configure(state="disabled")
        if not library_text.get().strip():
            version_status.set("Choose a version library directory.")
            return
        try:
            paths = client.versions(Path(library_text.get()))
            for path in paths:
                choices.insert("end", path.parent.name)
            if paths:
                index = paths.index(preferred) if preferred in paths else len(paths) - 1
                choices.selection_set(index)
                inspect()
            else:
                version_status.set("No complete published versions in this library.")
        except (OSError, PublicationError) as failure:
            version_status.set(str(failure))

    def play_selected(_event=None):
        if selected is not None:
            try:
                play(selected.manifest)
                version_status.set(f"Starting {selected.metadata['revision']} in a new temporary game session.")
            except (OSError, PublicationError, RecoveryError, StudioError) as failure:
                error("Launch failed", failure)

    ttk.Button(footer, text="Close", command=window.destroy).pack(side="left")
    ttk.Button(footer, text="Refresh", command=refresh).pack(side="left", padx=8)
    play_button = ttk.Button(footer, text="Play selected version", command=play_selected, state="disabled")
    play_button.pack(side="right")
    choices.bind("<<ListboxSelect>>", inspect)
    choices.bind("<Return>", play_selected)
    window.bind("<Escape>", lambda _: window.destroy())
    refresh()
    window.update_idletasks()
    window.lift()
    choices.focus_set()
    window.grab_set()
    parent.wait_window(window)
    return Path(library_text.get())
