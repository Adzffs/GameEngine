"""Recovery chooser; Tk is imported only when the editor opens this window."""
from pathlib import Path


def choose_recovery(store, executable: Path, source: Path, parent=None, quiet=False):
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
    from embervale_recovery import RecoveryError
    from embervale_world_editor import StudioError

    candidates = store.candidates(source)
    if not candidates:
        if not quiet:
            messagebox.showinfo("Recovery copies", store.diagnostics or
                "No closed recovery session was found for this map. Copies in another open editor are kept private.",
                parent=parent)
        return None
    window = tk.Toplevel(parent) if parent else tk.Tk()
    window.title("Recover map edits | Embervale Studio")
    window.geometry("740x520")
    window.minsize(640, 460)
    if parent:
        window.transient(parent)
    result = None
    frame = ttk.Frame(window, padding=16)
    frame.pack(fill="both", expand=True)
    ttk.Label(frame, text="Unfinished map edits", font=("", 14, "bold")).pack(anchor="w")
    ttk.Label(frame, text=f"Map: {source.name}", wraplength=680).pack(anchor="w", pady=(6, 10))
    ttk.Label(frame, text="Choose a recovery copy. Restoring saves a new draft and opens it for editing.",
              wraplength=680).pack(anchor="w")
    choices = tk.Listbox(frame, height=5, exportselection=False)
    choices.pack(fill="x", pady=10)
    details = tk.StringVar()
    ttk.Label(frame, textvariable=details, wraplength=680, justify="left").pack(anchor="w", fill="both", expand=True)
    footer = ttk.Frame(frame)
    footer.pack(fill="x", pady=(12, 0))

    def selection():
        picked = choices.curselection()
        return candidates[picked[0]] if picked else None

    def preview(_event=None):
        candidate = selection()
        restore_button.configure(state="disabled")
        if not candidate:
            return
        condition = ("The original has changed or is unavailable. This copy uses its saved base; "
                     "it will not merge with the current map." if candidate.base_changed()
                     else "The original still matches the saved base.")
        try:
            window.configure(cursor="watch")
            window.update_idletasks()
            changes = store.preview(candidate, executable)
            details.set(condition + "\n\n" + changes +
                        "\n\nThe recovered draft starts with a fresh undo history.")
            restore_button.configure(state="normal")
        except (OSError, RecoveryError, StudioError) as error:
            details.set(f"This copy could not be validated:\n{error}\nChoose an older copy if available.")
        finally:
            window.configure(cursor="")

    def restore(_event=None):
        nonlocal result
        candidate = selection()
        if not candidate or str(restore_button.cget("state")) == "disabled":
            return
        destination = filedialog.asksaveasfilename(parent=window,
            title="Save recovered draft as a new file",
            initialdir=str(source.parent if source.parent.is_dir() else Path.home()),
            initialfile=f"{source.stem}-recovered.json", defaultextension=".json",
            filetypes=[("World draft", "*.json")])
        if not destination:
            return
        try:
            window.configure(cursor="watch")
            window.update_idletasks()
            result = store.restore(candidate, executable, Path(destination))
            window.destroy()
        except (OSError, RecoveryError, StudioError) as error:
            messagebox.showerror("Recovery could not finish", str(error), parent=window)
            window.configure(cursor="")

    def discard():
        candidate = selection()
        if candidate and messagebox.askyesno("Discard recovery session?",
                "Discard all recovery copies from this closed editing session?\nOther sessions and map files stay available.",
                default="no", parent=window):
            try:
                store.resolve(candidate)
            except (OSError, RecoveryError) as error:
                messagebox.showerror("Recovery copies kept", str(error), parent=window)
                return
            refresh()

    def refresh(rescan=True):
        nonlocal candidates
        if rescan:
            candidates = store.candidates(source)
        choices.delete(0, "end")
        for candidate in candidates:
            choices.insert("end", candidate.label)
        if candidates:
            choices.selection_set(0)
            preview()
        else:
            window.destroy()

    ttk.Button(footer, text="Keep copies for later", command=window.destroy).pack(side="left")
    ttk.Button(footer, text="Discard session…", command=discard).pack(side="left", padx=8)
    restore_button = ttk.Button(footer, text="Restore selected copy…", command=restore)
    restore_button.pack(side="right")
    choices.bind("<<ListboxSelect>>", preview)
    choices.bind("<Double-Button-1>", restore)
    window.bind("<Escape>", lambda _: window.destroy())
    choices.bind("<Return>", restore)
    refresh(False)
    window.update_idletasks()
    window.deiconify()
    window.lift()
    choices.focus_set()
    if parent:
        window.grab_set()
        parent.wait_window(window)
    else:
        window.mainloop()
    return result
