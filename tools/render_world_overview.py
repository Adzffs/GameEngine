"""Render an exact starter-snapshot overview as a standalone SVG (no packages)."""

from __future__ import annotations

import argparse
from collections import Counter
from itertools import groupby
import json
from pathlib import Path
from xml.sax.saxutils import escape

TERRAIN = {"G": "#718b59", "D": "#c4a575", "S": "#98a3a4", "W": "#416f99"}
PEOPLE = {
    "npc.development_guide": ("M", "Mara Vale"),
    "npc.elia_reed": ("E", "Elia Reed"),
    "npc.nessa_thorne": ("N", "Nessa Thorne"),
    "npc.torren_ash": ("T", "Torren Ash"),
    "npc.brann_deepwell": ("B", "Brann Deepwell"),
    "npc.sera_flint": ("S", "Sera Flint"),
}
ENCOUNTERS = {
    "npc.passive_development_monster": ("M", "Mossling"),
    "npc.aggressive_development_monster": ("A", "Ashfang"),
    "npc.durable_development_monster": ("S", "Stoneback"),
    "npc.elite_development_monster": ("C", "Cinder Warden"),
}
SERVICES = {"station.bank": ("B", "Bank"), "station.furnace": ("F", "Furnace"),
            "station.crafting_table": ("C", "Crafting table")}


def render(world: dict, title: str) -> str:
    width, height = world["size"]["width"], world["size"]["height"]
    rows = world["terrain"]["rows"]
    if (type(width) is not int or type(height) is not int or
            not 1 <= width <= 256 or not 1 <= height <= 256 or
            len(rows) != height or any(len(row) != width or not set(row) <= TERRAIN.keys() for row in rows)):
        raise ValueError("Unsupported terrain shape or symbols")
    scale = 700 / max(width, height)
    left, top = 45, 112
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1150" height="910" viewBox="0 0 1150 910">',
             f'<title>{escape(title)}: authored terrain and placements</title>',
             '<desc>Flat map overview drawn from the actual world snapshot. Hover markers for exact identities and coordinates.</desc>',
             '<rect width="1150" height="910" rx="16" fill="#18252b"/>',
             '<g font-family="system-ui,Segoe UI,sans-serif" fill="#e8eedf">']

    def text(x, y, value, size=14, color="#e8eedf", weight="normal"):
        parts.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}">{escape(str(value))}</text>')

    text(45, 49, title, 32, weight="bold")
    counts = Counter(p["kind"] for p in world["placements"])
    text(45, 79, f'{width} × {height} tiles · {counts["resource"]} resources · {counts["station"]} stations · '
         f'{counts["npc_spawn"]} people · {counts["monster_spawn"]} encounters', 15, "#c0cebb")

    # Run-length rectangles preserve every authored tile without 10,000 SVG nodes.
    for y, row in enumerate(rows):
        x = 0
        for glyph, run in groupby(row):
            count = sum(1 for _ in run)
            parts.append(f'<rect x="{left + x * scale:g}" y="{top + y * scale:g}" '
                         f'width="{count * scale:g}" height="{scale:g}" fill="{TERRAIN[glyph]}"/>')
            x += count
    for x in range(0, width, 20):
        text(left + x * scale, top - 10, x, 11, "#c0cebb")
    for y in range(0, height, 20):
        text(13, top + y * scale + 5, y, 11, "#c0cebb")

    for p in sorted(world["placements"], key=lambda p: (p["kind"] != "resource", p["id"])):
        x, y = p["position"]["x"], p["position"]["y"]
        cx, cy = left + (x + .5) * scale, top + (y + .5) * scale
        kind, definition = p["kind"], p["definition_id"]
        parts.append(f'<g><title>{escape(p["id"])} · {escape(definition)} · ({x}, {y})</title>')
        label = ""
        if kind == "resource":
            tree = "tree" in definition
            color = "#c5e194" if tree else "#e2c6a1"
            if "coal" in definition:
                color = "#35434a"
            parts.append(f'<circle cx="{cx:g}" cy="{cy:g}" r="4" fill="{color}" stroke="#24352a" stroke-width="1.5"/>')
        elif kind == "station":
            label = SERVICES.get(definition, ("?", definition))[0]
            parts.append(f'<rect x="{cx-6:g}" y="{cy-6:g}" width="12" height="12" rx="2" fill="#f2c677" stroke="#24352a"/>')
        elif kind == "npc_spawn":
            label = PEOPLE.get(definition, ("?", definition))[0]
            parts.append(f'<circle cx="{cx:g}" cy="{cy:g}" r="7" fill="#edf0e4" stroke="#24352a"/>')
        else:
            label = ENCOUNTERS.get(definition, ("?", definition))[0]
            parts.append(f'<path d="M {cx:g} {cy-8:g} L {cx+8:g} {cy:g} L {cx:g} {cy+8:g} L {cx-8:g} {cy:g} Z" fill="#dd7b62" stroke="#422c28"/>')
        if label:
            parts.append(f'<text x="{cx:g}" y="{cy+3.5:g}" text-anchor="middle" font-size="9" font-weight="bold" fill="#243137">{escape(label)}</text>')
        parts.append('</g>')

    spawn = world["player_spawn"]
    sx, sy = left + (spawn["x"] + .5) * scale, top + (spawn["y"] + .5) * scale
    parts.append(f'<circle cx="{sx:g}" cy="{sy:g}" r="10" fill="none" stroke="#ffffff" stroke-width="2"/>')
    text(sx + 14, sy + 4, "Arrival", 13)

    legend_y = 126
    for heading, kind, catalogue in (("SERVICES", "station", SERVICES),
                                      ("PEOPLE", "npc_spawn", PEOPLE),
                                      ("ENCOUNTERS", "monster_spawn", ENCOUNTERS)):
        text(785, legend_y, heading, 13, "#c4b383", "bold")
        legend_y += 25
        for p in sorted((p for p in world["placements"] if p["kind"] == kind),
                        key=lambda p: p["initialization"]["order"]):
            letter, name = catalogue.get(p["definition_id"], ("?", p["definition_id"]))
            position = p["position"]
            text(785, legend_y, f'{letter}  {name}  ({position["x"]}, {position["y"]})', 14)
            legend_y += 23
        legend_y += 20
    text(785, legend_y, "GATHERING", 13, "#c4b383", "bold")
    legend_y += 25
    for definition, count in sorted(Counter(p["definition_id"] for p in world["placements"]
                                            if p["kind"] == "resource").items()):
        text(785, legend_y, f'{count} × {definition.split(".")[-1].replace("_", " ")}', 14)
        legend_y += 23
    for index, (glyph, name) in enumerate((("G", "Grass"), ("D", "Paths"), ("S", "Stone pads"), ("W", "Water"))):
        x = 45 + index * 165
        parts.append(f'<rect x="{x}" y="835" width="15" height="15" rx="3" fill="{TERRAIN[glyph]}"/>')
        text(x + 24, 847, name, 13)
    text(45, 880, "Building pads are walkable. Hover markers for exact IDs. Gameplay and pacing acceptance remain deferred.", 13, "#c0cebb")
    parts.append('</g></svg>\n')
    return "\n".join(parts)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--title", default="Starter world")
    args = parser.parse_args()
    world = json.loads(args.snapshot.read_text(encoding="utf-8"))
    args.output.write_text(render(world, args.title), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
