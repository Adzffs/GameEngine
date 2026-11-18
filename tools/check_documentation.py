#!/usr/bin/env python3
"""Check Embervale documentation without network access or third-party packages.

Run from any directory: python tools/check_documentation.py
For a documentation-only staging tree, pass --baseline-tree path/to/tree.json.
The optional tree is an unchanged GitHub recursive-tree response; its paths may
satisfy links to source files that are not included in the staging directory.

This is a deliberately bounded Markdown checker, not a complete CommonMark
implementation. It covers this project's inline/reference links, fenced blocks,
ATX/setext headings, explicit HTML anchors, and JSON examples. External URLs
are not fetched. Anchors in baseline-only files cannot be checked without bytes.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import html
import json
from pathlib import Path
import re
import sys
import unicodedata
from urllib.parse import unquote, urlsplit


TASK_PATTERN = r"(?:ENG|DOC|GAME|STUDIO|ART|QA|REL|WORLD|NET|FUT)-\d{3}"
TASK_RE = re.compile(r"\b" + TASK_PATTERN + r"\b")
LEGACY_STUDIO_IDS = {
    f"STUDIO-{number:03d}"
    for number in (
        1, 2, 3, *range(10, 15), *range(20, 25), *range(30, 34),
        *range(40, 44), *range(50, 55), *range(60, 64), *range(70, 75),
        *range(80, 85), *range(90, 95),
    )
}
BACKLOG_PATH = Path("docs/embervale/02-roadmap/BACKLOG.json")
FENCE_RE = re.compile(r"^ {0,3}(`{3,}|~{3,})(.*)$")
REFERENCE_RE = re.compile(r'^ {0,3}\[([^\]]+)\]:\s*(<[^>]+>|\S+)(?:\s+.*)?$')
INLINE_LINK_RE = re.compile(
    r'!?\[(?:\\.|[^\]\\])*\]\(\s*(<[^>]+>|[^\s()]+(?:\([^()]*\)[^\s()]*)*)'
    r'(?:\s+["\'][^\n]*?["\'])?\s*\)'
)
FULL_REFERENCE_RE = re.compile(r'!?\[([^\]\n]+)\]\[([^\]\n]*)\]')


@dataclass
class Markdown:
    path: Path
    text: str
    visible: str
    anchors: set[str]
    links: list[tuple[int, str]]


def no_duplicate_json_keys(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=no_duplicate_json_keys)


def normalise(value: object) -> str:
    return " ".join(str(value).replace(r"\|", "|").split())


def heading_slug(value: str) -> str:
    value = re.sub(r"!?(?:\[([^\]]+)\])\([^)]*\)", r"\1", value)
    value = re.sub(r"<[^>]*>", "", value)
    value = html.unescape(value).strip().lower().replace("`", "")
    value = "".join(
        char for char in value
        if char in "_- " or unicodedata.category(char)[0] in "LNM"
    )
    return value.replace(" ", "-")


def parse_markdown(path: Path, error) -> Markdown:
    text = path.read_text(encoding="utf-8")
    visible = []
    fence = None
    fence_start = 0
    block = []
    info = ""
    for number, line in enumerate(text.splitlines(), 1):
        match = FENCE_RE.match(line)
        if fence is not None:
            if match and match[1][0] == fence[0] and len(match[1]) >= len(fence) and not match[2].strip():
                if info == "json":
                    try:
                        json.loads("\n".join(block), object_pairs_hook=no_duplicate_json_keys)
                    except (ValueError, TypeError) as exc:
                        error(path, fence_start, f"invalid JSON example: {exc}")
                fence = None
            else:
                block.append(line)
            visible.append("")
            continue
        if match:
            fence, info, fence_start = match[1], match[2].strip().lower(), number
            block = []
            visible.append("")
        else:
            visible.append(line)
    if fence is not None:
        error(path, fence_start, "unclosed fenced code block")

    anchors, used_slugs = set(), Counter()
    for index, line in enumerate(visible):
        match = re.match(r"^ {0,3}#{1,6}\s+(.+?)(?:\s+#+\s*)?$", line)
        title = match[1] if match else None
        if title is None and index + 1 < len(visible) and line.strip():
            if re.match(r"^ {0,3}(?:={3,}|-{3,})\s*$", visible[index + 1]):
                title = line.strip()
        if title is not None:
            slug = heading_slug(title)
            suffix = used_slugs[slug]
            used_slugs[slug] += 1
            anchors.add(slug if not suffix else f"{slug}-{suffix}")
        anchors.update(re.findall(r'<[^>]+\b(?:id|name)=["\']([^"\']+)["\']', line))

    refs, links = {}, []
    for number, line in enumerate(visible, 1):
        match = REFERENCE_RE.match(line)
        if match:
            key = normalise(match[1]).casefold()
            if key in refs:
                error(path, number, f"duplicate reference definition: {match[1]}")
            refs[key] = match[2].strip("<>")
            links.append((number, refs[key]))
    for number, line in enumerate(visible, 1):
        if REFERENCE_RE.match(line):
            continue
        line = re.sub(r"(`+).*?\1", "", line)
        for match in INLINE_LINK_RE.finditer(line):
            links.append((number, match[1].strip("<>")))
        for match in FULL_REFERENCE_RE.finditer(line):
            key = normalise(match[2] or match[1]).casefold()
            if key not in refs:
                error(path, number, f"undefined reference: {match[2] or match[1]}")
    return Markdown(path, text, "\n".join(visible), anchors, links)


def markdown_tasks(document: Markdown, error) -> dict[str, dict]:
    rows, milestone = {}, None
    for number, line in enumerate(document.visible.splitlines(), 1):
        heading = re.match(r"^###\s+(R\d+)\b", line)
        if heading:
            milestone = heading[1]
        if not line.lstrip().startswith("|"):
            continue
        cells = [normalise(cell) for cell in re.split(r"(?<!\\)\|", line.strip().strip("|"))]
        if not cells or not re.fullmatch(TASK_PATTERN, cells[0]):
            continue
        task_id = cells[0]
        if task_id in rows:
            error(document.path, number, f"duplicate Markdown task ID: {task_id}")
        if len(cells) != 5:
            error(document.path, number, f"expected five task columns for {task_id}")
            continue
        rows[task_id] = {
            "milestone": milestone, "priority": cells[1], "title": cells[2],
            "dependencies": TASK_RE.findall(cells[3]), "acceptance": cells[4],
        }
    return rows


def check_backlog(root: Path, docs: dict[Path, Markdown], error, legacy_plans: list[Path]) -> int:
    path = root / BACKLOG_PATH
    try:
        data = read_json(path)
        tasks = data["tasks"]
        if not isinstance(tasks, list):
            raise ValueError("tasks must be an array")
    except (OSError, ValueError, TypeError, KeyError) as exc:
        error(path, 1, f"cannot read backlog: {exc}")
        return 0
    indexed = {}
    for task in tasks:
        if not isinstance(task, dict) or not isinstance(task.get("id"), str):
            error(path, 1, "task must be an object with a string ID")
            continue
        task_id = task["id"]
        if not re.fullmatch(TASK_PATTERN, task_id):
            error(path, 1, f"invalid task ID: {task_id}")
        if task_id in indexed:
            error(path, 1, f"duplicate JSON task ID: {task_id}")
        indexed[task_id] = task
        for field in ("milestone", "priority", "title", "dependencies", "acceptance", "status"):
            if field not in task:
                error(path, 1, f"{task_id}: missing {field}")
        if not re.fullmatch(r"R\d+", str(task.get("milestone", ""))):
            error(path, 1, f"{task_id}: invalid milestone")
        if task.get("priority") not in {"P0", "P1", "P2", "P3"}:
            error(path, 1, f"{task_id}: invalid priority")
        if task.get("status") == "complete" and not task.get("evidence"):
            error(path, 1, f"{task_id}: complete task has no evidence")
        dependencies = task.get("dependencies")
        if not isinstance(dependencies, list) or any(not isinstance(x, str) for x in dependencies):
            error(path, 1, f"{task_id}: dependencies must be a string array")
            task["dependencies"] = []
        elif len(dependencies) != len(set(dependencies)):
            error(path, 1, f"{task_id}: duplicate dependency")
    for task_id, task in indexed.items():
        for dependency in task.get("dependencies", []):
            if dependency not in indexed:
                error(path, 1, f"{task_id}: unknown dependency {dependency}")

    visited, active, reported_cycles = set(), [], set()
    def visit(task_id):
        if task_id in active:
            cycle = tuple(active[active.index(task_id):] + [task_id])
            if cycle not in reported_cycles:
                reported_cycles.add(cycle)
                error(path, 1, "dependency cycle: " + " -> ".join(cycle))
            return
        if task_id in visited or task_id not in indexed:
            return
        active.append(task_id)
        for dependency in indexed[task_id].get("dependencies", []):
            visit(dependency)
        active.pop()
        visited.add(task_id)
    for task_id in indexed:
        visit(task_id)

    md_path = path.with_suffix(".md")
    if md_path not in docs:
        error(md_path, 1, "Markdown backlog is missing")
        return len(tasks)
    rows = markdown_tasks(docs[md_path], error)
    if set(rows) != set(indexed):
        error(md_path, 1, f"Markdown/JSON ID mismatch: Markdown-only {sorted(set(rows)-set(indexed))}; JSON-only {sorted(set(indexed)-set(rows))}")
    for task_id in rows.keys() & indexed.keys():
        for field in ("milestone", "priority", "title", "acceptance"):
            if normalise(rows[task_id][field]) != normalise(indexed[task_id].get(field)):
                error(md_path, 1, f"{task_id}: Markdown/JSON mismatch for {field}")
        if rows[task_id]["dependencies"] != indexed[task_id].get("dependencies"):
            error(md_path, 1, f"{task_id}: Markdown/JSON dependency mismatch")

    legacy_ids = set(LEGACY_STUDIO_IDS)
    for legacy_path in legacy_plans:
        try:
            legacy_ids.update(re.findall(r"\bSTUDIO-\d{3}\b", legacy_path.read_text(encoding="utf-8")))
        except OSError as exc:
            error(legacy_path, 1, f"cannot read legacy plan: {exc}")
    missing = legacy_ids - indexed.keys()
    if missing:
        error(path, 1, f"legacy Studio IDs removed: {sorted(missing)}")
    for document in docs.values():
        for number, line in enumerate(document.visible.splitlines(), 1):
            for count in re.findall(r"\b(\d+)\s+(?:planning tasks|work packages|task records)\b", line):
                if int(count) != len(tasks):
                    error(document.path, number, f"declared task count {count} differs from {len(tasks)}")
            for task_id in TASK_RE.findall(line):
                if task_id not in indexed:
                    error(document.path, number, f"unknown referenced task: {task_id}")
    return len(tasks)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--baseline-tree", type=Path, help="GitHub recursive-tree JSON for unchanged staged source paths")
    parser.add_argument("--legacy-plan", type=Path, action="append", default=[], help="Additional historical plan whose Studio IDs must remain")
    args = parser.parse_args()
    root = args.root.resolve()
    errors, baseline_paths, baseline_sha = [], set(), None
    baseline_only, skipped_anchor_count, link_count = set(), 0, 0
    def error(path, line, message):
        try:
            label = str(path.relative_to(root))
        except ValueError:
            label = str(path)
        errors.append(f"{label}:{line}: {message}")
    if args.baseline_tree:
        try:
            baseline = read_json(args.baseline_tree)
            if baseline.get("truncated"):
                raise ValueError("baseline tree is truncated")
            baseline_paths = {item["path"] for item in baseline["tree"]}
            baseline_sha = baseline.get("sha")
        except (OSError, ValueError, TypeError, KeyError) as exc:
            error(args.baseline_tree, 1, f"invalid baseline tree: {exc}")

    paths = sorted({*root.glob("*.md"), *(root / "docs").rglob("*.md"), *(root / ".github").rglob("*.md")})
    docs = {}
    for path in paths:
        try:
            docs[path] = parse_markdown(path, error)
        except (OSError, UnicodeError) as exc:
            error(path, 1, f"cannot read Markdown: {exc}")
    if not docs:
        error(root, 1, "no Markdown documents found")
    for document in docs.values():
        for number, target in document.links:
            link_count += 1
            split = urlsplit(html.unescape(target))
            if split.scheme or split.netloc:
                prefix = f"/Adzffs/GameEngine/blob/{baseline_sha}/"
                if baseline_sha and split.netloc == "github.com" and split.path.startswith(prefix):
                    source_path = unquote(split.path[len(prefix):])
                    if source_path not in baseline_paths:
                        error(document.path, number, f"pinned source path absent from baseline: {source_path}")
                continue
            relative = unquote(split.path)
            destination = (document.path.parent / relative).resolve() if relative else document.path
            try:
                repository_path = destination.relative_to(root).as_posix()
            except ValueError:
                error(document.path, number, f"local link leaves repository: {target}")
                continue
            if not destination.exists():
                if repository_path in baseline_paths:
                    baseline_only.add(repository_path)
                    if split.fragment:
                        skipped_anchor_count += 1
                    continue
                error(document.path, number, f"missing local link target: {target}")
                continue
            if split.fragment:
                anchor = unquote(split.fragment)
                if destination in docs:
                    if anchor not in docs[destination].anchors:
                        error(document.path, number, f"missing Markdown anchor: {target}")
                elif destination.suffix.lower() in {".html", ".svg"}:
                    anchors = re.findall(r'\b(?:id|name)=["\']([^"\']+)["\']', destination.read_text(encoding="utf-8"))
                    if anchor not in anchors:
                        error(document.path, number, f"missing HTML/SVG anchor: {target}")
                else:
                    skipped_anchor_count += 1
    task_count = check_backlog(root, docs, error, args.legacy_plan)
    if errors:
        for message in errors:
            print("ERROR " + message, file=sys.stderr)
        print(f"FAIL: {len(errors)} error(s); {len(docs)} Markdown files, {link_count} links, {task_count} tasks.")
        return 1
    print(f"PASS: {len(docs)} Markdown files; {link_count} links; balanced fences and valid JSON examples; {task_count} matching backlog tasks; acyclic dependencies; {len(LEGACY_STUDIO_IDS)} legacy Studio IDs retained.")
    print(f"Scope: local links/anchors and supplied baseline paths only; external URLs were not fetched. Baseline-only targets: {len(baseline_only)}; anchors without local inspectable content: {skipped_anchor_count}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
