"""Private, bounded recovery journals for the authoritative Studio bridge.

Only original bytes and accepted typed commands are stored. Restoring replays
them through C++, validates the resulting state and exports a new JSON draft.
No record is applied over a changed source or a player save.
"""
from __future__ import annotations

import base64
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import tempfile
import uuid

MAX_SOURCE = 2 * 1024 * 1024
MAX_PACKET = 3 * 1024 * 1024
MAX_STORE = 32 * 1024 * 1024
MAX_SESSIONS = 128
HISTORY = 3
JOURNAL = re.compile(r"([0-9]{6})-[0-9a-f]{32}\.json\Z")
SESSION = re.compile(r"[0-9a-f]{64}-[0-9a-f]{32}\Z")
DIGEST = re.compile(r"[0-9a-f]{64}\Z")


class RecoveryError(RuntimeError):
    """Recovery failed without authorizing a destructive fallback."""


def canonical(value) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=True, allow_nan=False).encode("utf-8")


def state_digest(value) -> str:
    return hashlib.sha256(canonical(value)).hexdigest()


def default_directory() -> Path:
    if os.name == "nt":
        parent = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    else:
        parent = Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local" / "state"))
    return parent / "EmbervaleStudio" / "recovery"


def source_key(source: Path) -> str:
    return hashlib.sha256(os.fsencode(str(Path(source).resolve()))).hexdigest()


def read_regular(path: Path, limit: int) -> bytes:
    if path.is_symlink():
        raise RecoveryError("Recovery files must not be symbolic links")
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    try:
        info = os.fstat(descriptor)
        if not stat.S_ISREG(info.st_mode) or info.st_size > limit:
            raise RecoveryError("Recovery file is not regular or exceeds its size limit")
        with os.fdopen(descriptor, "rb", closefd=False) as stream:
            data = stream.read(limit + 1)
        if len(data) > limit:
            raise RecoveryError("Recovery file grew beyond its size limit")
        return data
    finally:
        os.close(descriptor)


class FileLock:
    """Nonblocking, process-lifetime OS lock; never delete an active lock inode."""

    def __init__(self, path: Path, create: bool = False):
        self.descriptor = -1
        if path.is_symlink():
            raise RecoveryError("Lock file must not be a symbolic link")
        flags = os.O_RDWR | getattr(os, "O_NOFOLLOW", 0)
        if create:
            flags |= os.O_CREAT
        descriptor = os.open(path, flags, 0o600)
        try:
            if not stat.S_ISREG(os.fstat(descriptor).st_mode):
                raise RecoveryError("Lock file must be regular")
            if os.name == "nt":
                import msvcrt
                msvcrt.locking(descriptor, msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except (OSError, RecoveryError):
            os.close(descriptor)
            raise RecoveryError("Another editor is active or its lock is unavailable") from None
        self.descriptor = descriptor

    def close(self):
        if self.descriptor >= 0:
            # Closing releases the lock even after a failed operation. A killed
            # process gets the same release from the OS, without age/PID guesses.
            os.close(self.descriptor)
            self.descriptor = -1

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()


def write_packet(path: Path, payload: dict):
    envelope = {"payload": payload, "sha256": state_digest(payload)}
    data = canonical(envelope)
    if len(data) > MAX_PACKET:
        raise RecoveryError("Recovery copy exceeds its 3 MiB limit")
    temporary = path.with_name(uuid.uuid4().hex + ".tmp")
    # Immutable generation names and an exclusively owned session prevent a
    # second cooperating writer from targeting this commit. Previous records
    # stay intact if the process stops before the rename.
    stream = temporary.open("xb")
    try:
        with stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        if path.exists() or path.is_symlink():
            raise RecoveryError("Recovery generation already exists")
        os.replace(temporary, path)
    finally:
        # Only this exclusively created stage is ours. Normal errors clean it;
        # process death may leave it, and readers never promote partial stages.
        temporary.unlink(missing_ok=True)


def read_packet(path: Path) -> dict:
    try:
        envelope = json.loads(read_regular(path, MAX_PACKET))
        if set(envelope) != {"payload", "sha256"}:
            raise ValueError("unexpected envelope")
        payload = envelope["payload"]
        if not isinstance(payload, dict) or state_digest(payload) != envelope["sha256"]:
            raise ValueError("checksum mismatch")
        return payload
    except (ValueError, TypeError, RecursionError, OverflowError) as error:
        raise RecoveryError(f"Recovery copy is damaged: {path.name}") from error


@dataclass(frozen=True)
class Candidate:
    folder: Path
    filename: str
    payload: dict
    skipped: int = 0

    @property
    def source(self) -> Path:
        return Path(self.payload["source"])

    @property
    def label(self) -> str:
        return f"{self.payload['created_utc']} — {len(self.payload['commands'])} commands — {self.folder.name[-8:]}"

    def base_changed(self) -> bool:
        try:
            return hashlib.sha256(read_regular(self.source, MAX_SOURCE)).hexdigest() != self.payload["base_sha256"]
        except (OSError, RecoveryError):
            return True


class RecoveryStore:
    def __init__(self, root: Path | None = None):
        selected = Path(root) if root is not None else default_directory()
        if selected.is_symlink():
            raise RecoveryError("Recovery directory must not be a symbolic link")
        self.root = selected.resolve()
        self.diagnostics = ""

    def _prepare(self):
        self.root.mkdir(mode=0o700, parents=True, exist_ok=True)
        if self.root.is_symlink() or not self.root.is_dir():
            raise RecoveryError("Recovery directory is unavailable")

    def _budget(self, extra: int):
        total = 0
        sessions = 0
        for folder in self.root.iterdir():
            if folder.is_symlink() or not folder.is_dir():
                continue
            sessions += 1
            if sessions > MAX_SESSIONS:
                raise RecoveryError("Recovery session limit reached; review older recovery copies")
            for index, entry in enumerate(folder.iterdir()):
                if index >= 16:
                    raise RecoveryError("Unexpected files in recovery session; inspect it before continuing")
                if entry.is_symlink() or not entry.is_file():
                    raise RecoveryError("Unexpected entry in recovery storage")
                total += entry.stat().st_size
                if total + extra > MAX_STORE:
                    raise RecoveryError("Recovery storage reached 32 MiB; review older recovery copies")
        return sessions

    def writer(self, bridge) -> "RecoveryWriter":
        self._prepare()
        # Session UUIDs isolate two windows editing the same source.
        folder = self.root / (source_key(bridge.source) + "-" + uuid.uuid4().hex)
        with FileLock(self.root / "budget.lock", create=True):
            self._collect_resolved()
            if self._budget(0) >= MAX_SESSIONS:
                raise RecoveryError("Recovery session limit reached; review older recovery copies")
            folder.mkdir(mode=0o700)
        return RecoveryWriter(self, folder, bridge.source)

    def _collect_resolved(self):
        # Resolved UUIDs are terminal: no writer ever reuses them. A scanner
        # may observe removal and simply skips it. Unknown/live data stays put.
        for folder in self.root.iterdir():
            if not SESSION.fullmatch(folder.name) or folder.is_symlink() or not (folder / "resolved.json").is_file():
                continue
            try:
                with FileLock(folder / "owner.lock"):
                    if read_packet(folder / "resolved.json") != {"format": 1, "session": folder.name, "resolved": True}:
                        continue
                    if {p.name for p in folder.iterdir()} != {"owner.lock", "resolved.json"}:
                        continue
                    (folder / "resolved.json").unlink()
                (folder / "owner.lock").unlink()
                folder.rmdir()
            except (OSError, RecoveryError):
                continue

    def _candidate(self, folder: Path, source: Path, filename: str | None = None) -> Candidate | None:
        if (folder / "resolved.json").exists():
            resolved = read_packet(folder / "resolved.json")
            if resolved == {"format": 1, "session": folder.name, "resolved": True}:
                return None
            raise RecoveryError("Invalid resolved-session receipt")
        entries = sorted((p for p in folder.iterdir() if JOURNAL.fullmatch(p.name)),
                         key=lambda p: p.name, reverse=True)
        if len(entries) > HISTORY + 1:
            raise RecoveryError("Too many recovery generations")
        skipped = 0
        for path in entries:
            if filename is not None and path.name != filename:
                continue
            try:
                payload = read_packet(path)
                fields = {"format", "session", "sequence", "source", "base_sha256",
                          "base_b64", "commands", "snapshot_sha256", "created_utc"}
                if set(payload) != fields or payload["format"] != 1 or payload["session"] != folder.name:
                    raise RecoveryError("Unsupported recovery format or session identity")
                if type(payload["sequence"]) is not int or payload["sequence"] != int(path.name[:6]):
                    raise RecoveryError("Recovery generation identity mismatch")
                if payload["source"] != str(source.resolve()) or source_key(source) != folder.name[:64]:
                    raise RecoveryError("Recovery source identity mismatch")
                for name in ("base_sha256", "snapshot_sha256"):
                    if not isinstance(payload[name], str) or not DIGEST.fullmatch(payload[name]):
                        raise RecoveryError("Invalid recovery digest")
                if not isinstance(payload["created_utc"], str):
                    raise RecoveryError("Invalid recovery date")
                datetime.fromisoformat(payload["created_utc"])
                commands = payload["commands"]
                if not isinstance(commands, list) or not 1 <= len(commands) <= 256:
                    raise RecoveryError("Invalid recovery command count")
                from embervale_world_editor import validate_replayable_editor_command, StudioError
                for command in commands:
                    if not isinstance(command, str) or len(command) > 256:
                        raise RecoveryError("Recovery command exceeds its limit")
                    try:
                        validate_replayable_editor_command(command)
                    except StudioError as error:
                        raise RecoveryError(str(error)) from error
                data = base64.b64decode(payload["base_b64"], validate=True)
                if not data or len(data) > MAX_SOURCE or hashlib.sha256(data).hexdigest() != payload["base_sha256"]:
                    raise RecoveryError("Recovery base bytes do not match their fingerprint")
                return Candidate(folder, path.name, payload, skipped)
            except (OSError, RecoveryError, ValueError, TypeError, KeyError):
                skipped += 1
        if skipped:
            self.diagnostics = "Damaged recovery copies were ignored; original files were preserved."
        return None

    def candidates(self, source: Path) -> list[Candidate]:
        self.diagnostics = ""
        if not self.root.exists():
            return []
        result = []
        prefix = source_key(source) + "-"
        for index, folder in enumerate(self.root.iterdir()):
            if index > MAX_SESSIONS + 1:
                self.diagnostics = "Recovery listing exceeded its session limit."
                break
            if not folder.name.startswith(prefix) or not SESSION.fullmatch(folder.name) or folder.is_symlink():
                continue
            try:
                with FileLock(folder / "owner.lock"):
                    filenames = sorted((p.name for p in folder.iterdir() if JOURNAL.fullmatch(p.name)), reverse=True)
                    if len(filenames) > HISTORY + 1:
                        raise RecoveryError("Too many recovery generations")
                    for filename in filenames:
                        candidate = self._candidate(folder, source, filename)
                        if candidate:
                            result.append(candidate)
            except (OSError, RecoveryError):
                # Live, unknown, linked or unreadable sessions are never taken
                # over, removed, or selected using an age/PID heuristic.
                continue
        return sorted(result, key=lambda c: (c.payload["created_utc"], c.payload["sequence"]), reverse=True)

    def resolve(self, candidate: Candidate):
        with FileLock(candidate.folder / "owner.lock"):
            current = self._candidate(candidate.folder, candidate.source, candidate.filename)
            if current is None or current.filename != candidate.filename:
                raise RecoveryError("Recovery selection changed; reopen the recovery list")
            self._resolve_owned(candidate.folder)

    def _resolve_owned(self, folder: Path):
        if not (folder / "resolved.json").exists():
            write_packet(folder / "resolved.json",
                         {"format": 1, "session": folder.name, "resolved": True})
        # Remove only known generations owned by this resolved session.
        # Keep the lock inode, avoiding an unlink/open race with a scanner.
        for entry in folder.iterdir():
            if JOURNAL.fullmatch(entry.name) or re.fullmatch(r"[0-9a-f]{32}\.tmp", entry.name):
                entry.unlink()

    def preview(self, candidate: Candidate, executable: Path) -> str:
        from embervale_world_editor import StudioBridge
        with FileLock(candidate.folder / "owner.lock"):
            current = self._candidate(candidate.folder, candidate.source, candidate.filename)
            if current is None or current.payload != candidate.payload:
                raise RecoveryError("Recovery selection changed")
            with tempfile.TemporaryDirectory(prefix="embervale-replay-") as temporary:
                base = Path(temporary) / "base.json"
                base.write_bytes(base64.b64decode(current.payload["base_b64"], validate=True))
                replay = StudioBridge(executable, base)
                result = replay._invoke(current.payload["commands"])
                if state_digest(result[5]) != current.payload["snapshot_sha256"]:
                    raise RecoveryError("Replayed world differs from the recovery checkpoint")
                before, after = replay.current_snapshot, result[5]
                tiles = sum(a != b for old, new in zip(before["terrain"]["rows"], after["terrain"]["rows"])
                            for a, b in zip(old, new))
                old = {p["id"]: p for p in before["placements"]}
                new = {p["id"]: p for p in after["placements"]}
                changed = [name for name in new.keys() & old.keys() if new[name] != old[name]]
                return (f"Changed terrain tiles: {tiles}\n"
                        f"Objects added: {len(new.keys() - old.keys())}   Removed: {len(old.keys() - new.keys())}\n"
                        f"Objects / spawn settings updated: {len(changed)}\n" +
                        "\n".join(changed[:8]))

    def restore(self, candidate: Candidate, executable: Path, destination: Path):
        """Return a verified new StudioBridge; source conflicts never get rebased."""
        from embervale_world_editor import (StudioBridge, StudioError,
                                           verify_saved_draft_receipt, verify_saved_world_parity)
        target = Path(destination).absolute()
        if (target.is_symlink() or target.resolve() == candidate.source.resolve() or
                target.suffix != ".json" or "\n" in str(target) or "\r" in str(target)):
            raise RecoveryError("Choose a new .json filename for the recovered copy")
        with FileLock(candidate.folder / "owner.lock"):
            current = self._candidate(candidate.folder, candidate.source, candidate.filename)
            if current is None or current.filename != candidate.filename or current.payload != candidate.payload:
                raise RecoveryError("Recovery selection changed; reopen the recovery list")
            # Reconstruct a private base, even if the original is missing,
            # corrupt, changed or blocked by a legacy save lock.
            with tempfile.TemporaryDirectory(prefix="embervale-replay-") as temporary:
                base = Path(temporary) / "base.json"
                base.write_bytes(base64.b64decode(current.payload["base_b64"], validate=True))
                try:
                    replay = StudioBridge(executable, base)
                    commands = current.payload["commands"]
                    result = replay._invoke(commands)
                    if state_digest(result[5]) != current.payload["snapshot_sha256"]:
                        raise RecoveryError("Replayed world differs from the recovery checkpoint")
                    saved = replay._invoke([*commands, "recover-copy " + str(target)])
                    content = verify_saved_draft_receipt(saved[4], target)
                    verify_saved_world_parity(result[5], content, result[1], result[3])
                    restored = StudioBridge(executable, target)
                    if state_digest(restored.current_snapshot) != current.payload["snapshot_sha256"]:
                        raise RecoveryError("Recovered copy changed when reopened")
                except StudioError as error:
                    raise RecoveryError(str(error)) from error
            self._resolve_owned(candidate.folder)
            return restored


class RecoveryWriter:
    def __init__(self, store: RecoveryStore, folder: Path, source: Path):
        self.store, self.folder, self.source = store, folder, source
        self.lock = FileLock(folder / "owner.lock", create=True)
        self.sequence = 0
        self.last_signature = None

    def checkpoint(self, bridge):
        if bridge.source != self.source:
            raise RecoveryError("Recovery session belongs to a different source")
        signature = (tuple(bridge.commands), state_digest(bridge.current_snapshot))
        if signature == self.last_signature:
            return
        if not bridge.dirty:
            return
        self.sequence += 1
        payload = {
            "format": 1, "session": self.folder.name, "sequence": self.sequence,
            "source": str(self.source), "base_sha256": bridge.hash,
            "base_b64": base64.b64encode(bridge.original_bytes).decode("ascii"),
            "commands": list(bridge.commands), "snapshot_sha256": signature[1],
            "created_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        }
        path = self.folder / f"{self.sequence:06d}-{uuid.uuid4().hex}.json"
        with FileLock(self.store.root / "budget.lock", create=True):
            self.store._budget(len(canonical({"payload": payload, "sha256": state_digest(payload)})))
            write_packet(path, payload)
            entries = sorted((p for p in self.folder.iterdir() if JOURNAL.fullmatch(p.name)),
                             key=lambda p: p.name, reverse=True)
            for older in entries[HISTORY:]:
                older.unlink()
        self.last_signature = signature

    def resolve(self):
        self.store._resolve_owned(self.folder)
        self.close()

    def close(self):
        self.lock.close()
