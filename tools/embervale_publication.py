"""Studio publication client. The C++ tool owns validation and world writing."""
from __future__ import annotations
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from embervale_recovery import read_regular, state_digest

DIGEST = re.compile(r"[0-9a-f]{64}\Z")
REVISION = re.compile(r"[a-z0-9](?:[a-z0-9-]{0,46}[a-z0-9])?\Z")
RECEIPT_KEYS = {"receipt_version", "source_sha256", "world_sha256", "world_schema_version",
                "validator_profile", "validator_revision"}
MANIFEST_KEYS = (RECEIPT_KEYS - {"receipt_version"}) | {"manifest_version", "revision", "world_file"}
RESERVED_NAMES = {"con", "prn", "aux", "nul"} | {f"{prefix}{n}" for prefix in ("com", "lpt") for n in range(1, 10)}


def valid_revision(value):
    return isinstance(value, str) and bool(REVISION.fullmatch(value)) and value not in RESERVED_NAMES


class PublicationError(RuntimeError):
    pass


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise PublicationError("Publication metadata contains duplicate fields")
        result[key] = value
    return result


def metadata(data: bytes | str, manifest=False) -> dict:
    if len(data) > 4096:
        raise PublicationError("Publication metadata exceeds 4 KiB")
    try:
        value = json.loads(data, object_pairs_hook=_pairs)
    except (ValueError, UnicodeError, RecursionError) as error:
        raise PublicationError("Publication metadata is not valid JSON") from error
    if not isinstance(value, dict) or set(value) != (MANIFEST_KEYS if manifest else RECEIPT_KEYS):
        raise PublicationError("Publication metadata fields are not supported")
    version = value["manifest_version" if manifest else "receipt_version"]
    if type(version) is not int or version != 1:
        raise PublicationError("Publication metadata version is not supported")
    for field in ("source_sha256", "world_sha256", "validator_revision"):
        if not isinstance(value[field], str) or not DIGEST.fullmatch(value[field]):
            raise PublicationError("Invalid publication fingerprint")
    if (value["validator_profile"] != "starter-runtime-v1" or
            type(value["world_schema_version"]) is not int or
            value["world_schema_version"] not in (1, 2, 3, 4)):
        raise PublicationError("Unsupported world validation profile/schema")
    if manifest and (value["world_file"] != "world.json" or not valid_revision(value["revision"])):
        raise PublicationError("Invalid published version identity")
    return value


@dataclass(frozen=True)
class PublishedVersion:
    manifest: Path
    metadata: dict


class PublicationClient:
    def __init__(self, executable: Path):
        self.executable = Path(executable).resolve()
        self.validated = None

    def _invoke(self, *arguments) -> str:
        command = [str(self.executable), *map(str, arguments)]
        if self.executable.suffix == ".py":
            command.insert(0, sys.executable)
        try:
            result = subprocess.run(command, text=True, encoding="utf-8", errors="replace",
                                    capture_output=True, timeout=20, check=False)
        except (OSError, subprocess.SubprocessError) as error:
            raise PublicationError(f"Build/select EmbervaleWorldPublish: {error}") from error
        if result.returncode or result.stderr.strip():
            raise PublicationError(result.stderr.strip() or "The publishing tool failed")
        return result.stdout

    def _saved_source(self, bridge) -> tuple[Path, bytes]:
        from embervale_world_editor import verify_saved_world_parity
        if bridge.dirty:
            raise PublicationError("Save a new draft before validating or publishing these edits")
        path = bridge.last_saved or bridge.source
        content = read_regular(path, 2 * 1024 * 1024)
        verify_saved_world_parity(bridge.current_snapshot, content, bridge.placements, bridge.terrain)
        return Path(path).resolve(), content

    def validate(self, bridge) -> dict:
        self.validated = None
        source, content = self._saved_source(bridge)
        receipt = metadata(self._invoke("--validate", source))
        if receipt["source_sha256"] != hashlib.sha256(content).hexdigest():
            raise PublicationError("Draft changed during validation; validate it again")
        if receipt["world_schema_version"] != bridge.current_snapshot["schema_version"]:
            raise PublicationError("Validation receipt does not describe this draft schema")
        self.validated = (source, receipt, state_digest(bridge.current_snapshot))
        return receipt

    def publish(self, bridge, library: Path, revision: str) -> PublishedVersion:
        if not valid_revision(revision):
            raise PublicationError("Use 1–48 lowercase letters/digits/hyphens, starting and ending with a letter or digit")
        if self.validated is None:
            raise PublicationError("Validate this draft before publishing")
        source, content = self._saved_source(bridge)
        validated_source, receipt, snapshot = self.validated
        if (source != validated_source or hashlib.sha256(content).hexdigest() != receipt["source_sha256"] or
                state_digest(bridge.current_snapshot) != snapshot):
            self.validated = None
            raise PublicationError("Draft changed since validation; validate it again")
        library = Path(library).absolute()
        with tempfile.TemporaryDirectory(prefix="embervale-publication-receipt-") as temporary:
            receipt_path = Path(temporary) / "receipt.json"
            receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
            result = metadata(self._invoke("--publish", source, "--receipt", receipt_path,
                "--library", library, "--revision", revision), manifest=True)
        expected = {k: v for k, v in receipt.items() if k != "receipt_version"}
        expected.update(manifest_version=1, revision=revision, world_file="world.json")
        if result != expected:
            raise PublicationError("Published version differs from its validation receipt")
        version = self.inspect(library / revision / "manifest.json")
        if version.metadata != expected:
            raise PublicationError("Published metadata changed while verifying")
        return version

    def inspect(self, manifest: Path) -> PublishedVersion:
        manifest = Path(manifest).absolute()
        value = metadata(self._invoke("--inspect", manifest), manifest=True)
        actual = metadata(read_regular(manifest, 4096), manifest=True)
        if actual != value or manifest.name != "manifest.json" or manifest.parent.name != value["revision"]:
            raise PublicationError("Published version changed or has an invalid package layout")
        return PublishedVersion(manifest, value)

    @staticmethod
    def versions(library: Path) -> list[Path]:
        library = Path(library).absolute()
        if library.is_symlink():
            raise PublicationError("Select a real version library directory")
        if not library.exists():
            return []
        versions = []
        for count, entry in enumerate(library.iterdir()):
            if count >= 256:
                raise PublicationError("This library exceeds the 256-entry browser limit; select a smaller library")
            if (valid_revision(entry.name) and not entry.is_symlink() and entry.is_dir() and
                    not (entry / "manifest.json").is_symlink() and (entry / "manifest.json").is_file()):
                versions.append(entry / "manifest.json")
        return sorted(versions)
