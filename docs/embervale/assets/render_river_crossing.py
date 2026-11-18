#!/usr/bin/env python3
"""Render the PROPOSED River Crossing planning diagram from its Markdown spec.

This is a documentation plot, not an engine or Studio renderer. Requires
Python 3, matplotlib and numpy. Run from any directory:
    python3 docs/embervale/assets/render_river_crossing.py

Authored geometry and anchor coordinates are parsed from RIVER_CROSSING.md.
Label offsets, colours and marker sizes are presentation choices only.
"""

from pathlib import Path
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.lines import Line2D
from matplotlib.patches import Patch, Rectangle
import numpy as np


HERE = Path(__file__).resolve().parent
SPEC = HERE.parent / "05-game" / "RIVER_CROSSING.md"
TEXT = SPEC.read_text(encoding="utf-8")
COORD = re.compile(r"\((\d+),(\d+)\)")


def section(start, end):
    return TEXT.split(start, 1)[1].split(end, 1)[0]


def xy_named(pattern):
    match = re.search(pattern, TEXT, re.I)
    if match is None:
        raise ValueError(f"Missing coordinate: {pattern}")
    return tuple(map(int, match.groups()))


def table_rows(body):
    for line in body.splitlines():
        if line.startswith("|"):
            yield [cell.strip() for cell in line.strip("|").split("|")]


# Read every game-space coordinate from the document.
river = []
for row in table_rows(section("## 3. Terrain contours", "## 4. Roads")):
    y = re.fullmatch(r"`y=(\d+)\.\.(\d+)`", row[0])
    x = re.fullmatch(r"`x=(\d+)\.\.(\d+)`", row[1]) if len(row) > 1 else None
    if x and y:
        river.append((*map(int, x.groups()), *map(int, y.groups())))
causeways = [tuple(map(int, match)) for match in re.findall(
    r"`x=(\d+)\.\.(\d+), y=(\d+)\.\.(\d+)`",
    section("After painting the channel", "Water remains impassable"))]
roads = {}
for row in table_rows(section("## 4. Roads", "Add short passable connectors")):
    if len(row) >= 2:
        points = [tuple(map(int, pair)) for pair in COORD.findall(row[1])]
        if points:
            roads[row[0]] = points
buildings = {}
for row in table_rows(section("## 5. Building", "Place the furnace")):
    if len(row) < 2:
        continue
    match = re.fullmatch(r"`x=(\d+)\.\.(\d+), y=(\d+)\.\.(\d+)`", row[1])
    if match:
        buildings[row[0]] = tuple(map(int, match.groups()))
resources = {}
for row in table_rows(section("### Proposed resource coordinates", "Keep at least one")):
    if len(row) >= 2:
        points = [tuple(map(int, pair)) for pair in COORD.findall(row[1])]
        if points:
            resources[row[0]] = points
npcs = {
    name: xy_named(name + r" `\((\d+),(\d+)\)`")
    for name in ("Mara", "Elia", "Nessa", "Brann", "Sera")
}
npcs["Torren"] = xy_named(r"Torren stands at `\((\d+),(\d+)\)`")
stations = {
    "Bank": xy_named(r"Bank `\((\d+),(\d+)\)`"),
    "Furnace": xy_named(r"furnace at `\((\d+),(\d+)\)`"),
    "Crafting table": xy_named(r"crafting table at `\((\d+),(\d+)\)`"),
}
approaches = {
    "Bank": xy_named(r"Bank approach `\((\d+),(\d+)\)`"),
    "Furnace": xy_named(r"furnace at `\(\d+,\d+\)` with approach `\((\d+),(\d+)\)`"),
    "Crafting table": xy_named(r"crafting table at `\(\d+,\d+\)` with approach `\((\d+),(\d+)\)`"),
}
encounters = {
    "Mossling": xy_named(r"Mossling begins at `\((\d+),(\d+)\)`"),
    "Ashfang": xy_named(r"Ashfang begins at `\((\d+),(\d+)\)`"),
    "Stoneback": xy_named(r"Stoneback at `\((\d+),(\d+)\)`"),
    "Cinder Warden": xy_named(r"Cinder Warden at `\((\d+),(\d+)\)`"),
}
arrival = xy_named(r"Proposed test-player arrival \| `\((\d+),(\d+)\)`")
assert len(river) == 5 and len(causeways) == 2
assert len(roads) == 6 and len(buildings) == 7
assert sum(map(len, resources.values())) == 18
assert len(npcs) == 6 and len(stations) == 3 and len(encounters) == 4
all_points = [arrival, *npcs.values(), *stations.values(), *approaches.values(), *encounters.values()]
all_points += [point for points in resources.values() for point in points]
all_points += [point for points in roads.values() for point in points]
assert all(0 <= x < 100 and 0 <= y < 100 for x, y in all_points)
assert all(0 <= x0 <= x1 < 100 and 0 <= y0 <= y1 < 100
           for x0, x1, y0, y1 in [*river, *causeways, *buildings.values()])


def rect_tiles(box):
    x0, x1, y0, y1 = box
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def segment_tiles(a, b, radius=0):
    assert a[0] == b[0] or a[1] == b[1], (a, b)
    return {
        (u, v)
        for x in range(min(a[0], b[0]), max(a[0], b[0]) + 1)
        for y in range(min(a[1], b[1]), max(a[1], b[1]) + 1)
        for u in range(x - radius, x + radius + 1)
        for v in range(y - radius, y + radius + 1)
        if 0 <= u < 100 and 0 <= v < 100
    }


water = set().union(*(rect_tiles(box) for box in river))
crossings = set().union(*(rect_tiles(box) for box in causeways))
water -= crossings
road_tiles = set()
for points in roads.values():
    for a, b in zip(points, points[1:]):
        road_tiles |= segment_tiles(a, b, radius=1)
assert not water & road_tiles
all_resources = {point for points in resources.values() for point in points}
assert len(all_resources) == 18
assert not all_resources & (water | road_tiles)

# Required short connections are drawn as dashed links to the nearest existing
# road tile. They reserve access, not an implemented width/collision contract.
connectors = []
for point in [npcs["Mara"], npcs["Nessa"], npcs["Sera"], npcs["Brann"], *approaches.values()]:
    destination = min(road_tiles, key=lambda p: (abs(p[0]-point[0])+abs(p[1]-point[1]), p))
    assert point[0] == destination[0] or point[1] == destination[1]
    connectors.append((point, destination))

terrain = np.zeros((100, 100), dtype=np.uint8)
for x, y in water:
    terrain[y, x] = 1
for x, y in road_tiles:
    terrain[y, x] = 2
for x, y in crossings:
    terrain[y, x] = 3

COLORS = {
    "ink": "#20323d", "muted": "#58717b", "ground": "#eff3e9",
    "water": "#7eb7cf", "road": "#d7bd88", "crossing": "#b77c43",
    "building": "#d5dce3", "building_edge": "#7c8a99", "npc": "#2e5caa",
    "station": "#8f4f8b", "enemy": "#a93d40", "normal": "#3f7955",
    "oak": "#80963e", "willow": "#248c83", "passive": "#b68930",
}
matplotlib.rcParams.update({
    "font.family": "DejaVu Sans", "font.size": 10,
    "svg.fonttype": "none", "axes.labelcolor": COLORS["ink"],
    "text.color": COLORS["ink"], "xtick.color": COLORS["muted"],
    "ytick.color": COLORS["muted"], "axes.edgecolor": "#a8b5b9",
})
fig = plt.figure(figsize=(18, 13), facecolor="#fcfcf8")
fig.text(.055, .954, "RIVER CROSSING", fontsize=27, weight="bold")
fig.text(.055, .922, "PROPOSED FLAT BLOCKOUT  /  100 × 100 tiles", fontsize=14, weight="bold", color="#8c5b27")
fig.text(.055, .895, "A precise planning diagram from RIVER_CROSSING.md — not a current game screenshot or an editor render.", fontsize=11)
ax = fig.add_axes([.055, .12, .62, .745])


def draw_base(target, limits=None, fine_grid=False):
    target.imshow(terrain, origin="upper", extent=(-.5, 99.5, 99.5, -.5), interpolation="nearest",
                  cmap=ListedColormap([COLORS[k] for k in ("ground", "water", "road", "crossing")]), vmin=0, vmax=3)
    for index, (name, box) in enumerate(buildings.items(), 1):
        x0, x1, y0, y1 = box
        target.add_patch(Rectangle((x0-.5, y0-.5), x1-x0+1, y1-y0+1,
                                  facecolor=COLORS["building"], edgecolor=COLORS["building_edge"],
                                  linewidth=1.0, hatch="//", alpha=.85, zorder=3))
        if limits is None:
            target.text(x0+.25, y0+1, f"B{index}", fontsize=8, va="top", weight="bold", zorder=7)
    for a, b in connectors:
        target.plot([a[0], b[0]], [a[1], b[1]], color="#9d7140", linewidth=1.8, linestyle=(0, (2, 1)), zorder=4)
    if limits:
        target.set_xlim(limits[0], limits[1]); target.set_ylim(limits[3], limits[2])
        target.set_xticks(np.arange(limits[0], limits[1]+.1, 2))
        target.set_yticks(np.arange(limits[2], limits[3]+.1, 2))
        target.set_xticks(np.arange(limits[0]-.5, limits[1]+.5, 1), minor=True)
        target.set_yticks(np.arange(limits[2]-.5, limits[3]+.5, 1), minor=True)
    else:
        target.set_xlim(-.5, 99.5); target.set_ylim(99.5, -.5)
        ticks = list(range(0, 100, 10))+[99]
        target.set_xticks(ticks); target.set_yticks(ticks)
        target.set_xticks(np.arange(-.5, 100, 1), minor=True)
        target.set_yticks(np.arange(-.5, 100, 1), minor=True)
    target.grid(which="major", color="#78918b", linewidth=.55, alpha=.22)
    target.grid(which="minor", color="#78918b", linewidth=.25, alpha=.13 if not fine_grid else .3)
    target.tick_params(which="minor", length=0)
    target.set_aspect("equal")


draw_base(ax)
ax.set_xlabel("x → EAST   |   tile coordinates", fontsize=11, labelpad=9)
ax.set_ylabel("y → SOUTH   |   north is up", fontsize=11, labelpad=10)
resource_styles = {
    "Normal tree": ("^", COLORS["normal"], "N"), "Oak tree": ("^", COLORS["oak"], "O"),
    "Willow tree": ("^", COLORS["willow"], "W"), "Copper rock": ("D", "#aa623c", "Cu"),
    "Tin rock": ("D", "#83959e", "Sn"), "Iron rock": ("D", "#526774", "Fe"),
    "Coal rock": ("D", "#333f43", "Co"),
}
for kind, points in resources.items():
    marker, color, code = resource_styles[kind]
    for point in points:
        ax.scatter(*point, marker=marker, s=53, color=color, edgecolor="white", linewidth=.65, zorder=8)
        ax.annotate(code, point, xytext=(4, -8), textcoords="offset points", fontsize=7.8, color=color, weight="bold", zorder=9)


def draw_anchors(target, scale=1):
    for name, point in npcs.items():
        target.scatter(*point, s=45*scale, color=COLORS["npc"], edgecolor="white", linewidth=.9, zorder=10)
    for name, point in stations.items():
        target.scatter(*point, marker="s", s=52*scale, color=COLORS["station"], edgecolor="white", linewidth=.9, zorder=11)
    for point in approaches.values():
        target.scatter(*point, marker="s", s=22*scale, facecolors="none", edgecolors=COLORS["station"], linewidth=1, zorder=9)


draw_anchors(ax)
for name, point in encounters.items():
    passive = name == "Mossling"
    color = COLORS["passive"] if passive else COLORS["enemy"]
    ax.scatter(*point, marker="P" if passive else "X", s=76, color=color, edgecolor="white", linewidth=.7, zorder=10)
ax.scatter(*arrival, marker="*", s=165, color="#d3a71f", edgecolor="#6a5214", linewidth=.8, zorder=12)


def label(text, point, location, color=None, size=10, align="left"):
    ax.annotate(text, point, xytext=location, textcoords="data", fontsize=size, ha=align, va="center",
                weight="medium", color=color or COLORS["ink"],
                bbox=dict(boxstyle="round,pad=.3", fc="#fcfcf8", ec="none", alpha=.94),
                arrowprops=dict(arrowstyle="-", color=color or COLORS["muted"], linewidth=.85,
                                connectionstyle="angle3,angleA=0,angleB=90"), zorder=15)


label("Arrival\n(5,28)", arrival, (1, 34), size=9)
label("Mara", npcs["Mara"], (4, 15))
label("Bank + Elia\nsee detail", stations["Bank"], (26, 13), size=10)
label("Sera", npcs["Sera"], (84, 13))
label("Nessa", npcs["Nessa"], (2, 38))
label("Workshop cluster\nsee detail", stations["Furnace"], (58, 36), size=10)
label("Brann", npcs["Brann"], (87, 47))
label("Mossling\npassive", encounters["Mossling"], (21, 83), color=COLORS["passive"], size=10)
label("Ashfang", encounters["Ashfang"], (61, 83), color=COLORS["enemy"])
label("Stoneback", encounters["Stoneback"], (83, 75), color=COLORS["enemy"])
label("Cinder Warden", encounters["Cinder Warden"], (65, 94), color=COLORS["enemy"])
for number, box in enumerate(causeways, 1):
    x0, x1, y0, y1 = box
    point = ((x0+x1)/2, (y0+y1)/2)
    label(f"{['North','South'][number-1]} causeway", point, (51 if number == 1 else 54, 20 if number == 1 else 71), size=8.5)
ax.text(3, 93, "18 resources · 3 stations\n6 friendly NPCs · 4 encounter anchors", fontsize=9,
        bbox=dict(boxstyle="round,pad=.5", fc="#fcfcf8", ec="#d8e1d5"), zorder=20)

legend_ax = fig.add_axes([.705, .625, .26, .255]); legend_ax.axis("off")
legend_ax.text(0, 1, "READING THE PLAN", fontsize=13, weight="bold", va="top")
handles = [
    Patch(facecolor=COLORS["water"], label="Water: blocked ground"),
    Patch(facecolor=COLORS["road"], label="Road: 3-tile corridor"),
    Patch(facecolor=COLORS["crossing"], label="Causeway: passable land"),
    Patch(facecolor=COLORS["building"], hatch="//", edgecolor=COLORS["building_edge"], label="Building reservation"),
    Line2D([], [], color="#9d7140", linestyle="--", label="Required service connection"),
    Line2D([], [], marker="o", color="none", markerfacecolor=COLORS["npc"], label="Friendly NPC anchor"),
    Line2D([], [], marker="s", color="none", markerfacecolor=COLORS["station"], label="Station anchor"),
    Line2D([], [], marker="s", color="none", markeredgecolor=COLORS["station"], markerfacecolor="none", label="Required approach tile"),
    Line2D([], [], marker="^", color="none", markerfacecolor=COLORS["normal"], label="Tree: N normal / O oak / W willow"),
    Line2D([], [], marker="D", color="none", markerfacecolor="#526774", label="Rock: Cu copper / Sn tin / Fe iron / Co coal"),
    Line2D([], [], marker="P", color="none", markerfacecolor=COLORS["passive"], label="Passive encounter"),
    Line2D([], [], marker="X", color="none", markerfacecolor=COLORS["enemy"], label="Aggressive / advanced encounter"),
]
legend_ax.legend(handles=handles, loc="upper left", bbox_to_anchor=(-.02, .93), frameon=False,
                 fontsize=9.2, handlelength=1.5, handletextpad=.8, labelspacing=.55)

detail1 = fig.add_axes([.705, .416, .118, .165])
draw_base(detail1, (29, 38, 22, 31), True); draw_anchors(detail1, 1.2)
detail1.set_title("BANK FRONTAGE", fontsize=10, weight="bold", loc="left", pad=10)
detail1.annotate("Bank", stations["Bank"], xytext=(-18, 18), textcoords="offset points", fontsize=8, weight="bold")
detail1.annotate("Elia", npcs["Elia"], xytext=(5, 10), textcoords="offset points", fontsize=8, weight="bold")
detail1.annotate("approach", approaches["Bank"], xytext=(-6, -20), textcoords="offset points", fontsize=7.5)
detail1.tick_params(labelsize=7)
detail2 = fig.add_axes([.842, .416, .123, .165])
draw_base(detail2, (65, 76, 46, 55), True); draw_anchors(detail2, 1.2)
detail2.set_title("WORKSHOP YARD", fontsize=10, weight="bold", loc="left", pad=10)
detail2.annotate("Furnace", stations["Furnace"], xytext=(-17, 18), textcoords="offset points", fontsize=7.8, weight="bold")
detail2.annotate("Crafting", stations["Crafting table"], xytext=(-3, 18), textcoords="offset points", fontsize=7.8, weight="bold")
detail2.annotate("Torren", npcs["Torren"], xytext=(-9, -21), textcoords="offset points", fontsize=7.8, weight="bold")
detail2.tick_params(labelsize=7)

notes = fig.add_axes([.705, .14, .26, .24]); notes.axis("off")
notes.text(0, 1, "RESERVED BUILDING FOOTPRINTS", fontsize=11, weight="bold", va="top")
for i, name in enumerate(buildings, 1):
    short = {"Records store and bank frontage": "Records store / bank frontage", "Two-home residential reserve": "Two-home residential reserve"}.get(name, name)
    notes.text(0, 1-i*.09, f"B{i}  {short}", fontsize=9.7, va="top")
notes.text(0, .22, "Rectangles reserve space; they are not wall masks.\nMarkers show anchors, not entity footprints.\nNo height, bridges above water, or threat-radius\nclaims are encoded in this diagram.", fontsize=9, color=COLORS["muted"], va="top", linespacing=1.5)
fig.text(.055, .055, "SOURCE  docs/embervale/05-game/RIVER_CROSSING.md   |   Candidate only   |   18 November 2026", fontsize=9.5, color=COLORS["muted"])
fig.text(.055, .033, "Geometry sanity-checked for bounds and river/road/resource overlap. Runtime pathfinding, interaction reachability and balance still require testing.", fontsize=9, color=COLORS["muted"])

HERE.mkdir(parents=True, exist_ok=True)
png = HERE / "river-crossing-proposed.png"
svg = HERE / "river-crossing-proposed.svg"
fig.savefig(png, dpi=180, facecolor=fig.get_facecolor())
fig.savefig(svg, facecolor=fig.get_facecolor(), metadata={"Date": None})
plt.close(fig)
print(f"Wrote {png.name} and {svg.name}; all authored coordinates parsed from {SPEC.name}.")
