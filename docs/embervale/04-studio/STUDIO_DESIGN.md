# Embervale Studio product and workspace design

**Current implementation:** Shared move/paint/place/remove/spawn settings, history, guarded new drafts, reopen, isolated playtests and overlays exist. See [current status](../01-current-state/CURRENT_STATUS.md), [visual editor](VISUAL_WORLD_EDITOR.md) and the [project review](../06-development/PROJECT_REVIEW_2026-11-20.md). Private [accepted-edit recovery](EDITOR_RECOVERY.md) is now implemented. [Fixed world publication and selection](WORLD_PUBLICATION.md) are also implemented. In-place replacement and broader gameplay/asset workspaces remain open.


**Status:** A bounded world editor is implemented. The broader product/workspaces below remain a target design, not a completed release.\
**Updated:** 20 November 2026\
**Purpose:** Define a practical editor that Adam and AI agents can use through the same reliable operations.

## 1. Product outcome

Embervale Studio should let a creator change the world without repeatedly editing C++ coordinates. A creator should be able to place a tree, see its occupied tile and approach area, validate access, inspect a clear change summary, save the draft and test the result in the game. An AI agent should be able to perform the same task from typed data and receive the same diagnostics.

The first product is deliberately bounded: open the exported development region, make one reversible terrain/resource change, validate, preview and play it. The wider toolkit grows from that completed path. A large collection of empty tabs does not satisfy the release gate.

### The first acceptance story

From a documented clean environment, inspect a specific region revision, move one existing tree to a valid tile, review the resulting collision and route changes, undo and redo it, save, reopen in the GUI and launch a test world from the same content hash. The original terrain is restored at the old location, the tree keeps its authored ID, invalid moves are rejected or retained only as an explicitly invalid draft where supported, and no normal player save is modified.

The default world uses compiled placements and code-drawn presentation. [Explicit snapshot loading](STARTER_WORLD_IMPORT.md) now accepts supported coordinates and terrain through the real runtime; this is the integration boundary for the planned shared editor operations. [Sources: DevelopmentWorldContent.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Development/DevelopmentWorldContent.cpp), [Graphics.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp).

## 2. Workspace map

| Workspace | Creator task | First useful slice | Later capability |
|---|---|---|---|
| World | Shape terrain and place content | Grid, selection, one-tile paint, resource move | Larger brushes, region connections, supported footprints |
| Entities and spawns | Place services, characters and encounters | Existing definitions and spawn records | Author spawn zones, schedules and encounter groups when runtime supports them |
| Assets | Connect visual files to stable definitions | One terrain tile and one tree sprite with fallback | Atlases, animation previews, variants and provenance checks |
| Dialogue and quests | Author conversations and progression | Inspect existing graphs, then edit one proven content type | Dependency view, simulated quest states and localisation keys |
| Items and recipes | Tune equipment, gathering and crafting | Existing definition inspector and validation | Pattern editor, comparisons and recipe discovery tools |
| Enemies and rewards | Tune encounter/reward definitions | Existing enemy and reward catalogue adapters | Controlled combat simulations and distribution reports |
| Skills, shops and economy | Understand progression and prices | Read-only tables and reproducible reports | Reviewed editing and anti-arbitrage checks |
| Debug and playtest | Explain behaviour and diagnose a map | Collision/routes, diagnostics, content hash and test launch | Profiling, encounter overlays and reproducible scenarios |

These workspaces are roadmap categories, not promises that all must ship together. Each gameplay editor requires its own schema adapter, reference validation, runtime loading path and acceptance test before it is considered complete.

## 3. First GUI layout

A proposed first layout has a central map viewport, a narrow palette on the left, a selected-object inspector on the right and a diagnostics/history area below. A top bar shows the region, current content hash or short revision, dirty state and available validation status. Avoid a large dashboard that hides the map.

The viewport supports pan, zoom, grid display and tile coordinates. Selection outlines distinguish authored footprint from sprite extent. A blocked-tile overlay and player-spawn marker make the important gameplay consequences visible. The inspector displays durable ID, definition, tile position, surface and relevant validated properties; temporary runtime IDs belong in debug information, not the main identity field.

Reuse or extract the current camera-to-world conversion carefully. Zoomed clicks must select the same tile that the renderer displays, including edge coordinates and negative/out-of-bounds pointer positions. Current camera and rendering code lives in the client layer and must not become an SDL requirement for headless commands. [Sources: Graphics.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Graphics/Graphics.cpp), [CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt).

### Controls to ship first

| Action | Expected behaviour |
|---|---|
| Select | Click a placement or choose it by ID/name from a list |
| Inspect | Show editable authored fields and read-only derived properties |
| Move | Show candidate tile and validation result before commit |
| Paint | Change bounded terrain cells through a command |
| Undo/redo | Reverse or restore a complete command group |
| Validate | Show errors with links to relevant tiles/fields |
| Save draft | Persist recoverable authored work with conflict checks |
| Preview/playtest | Generate evidence or launch the explicitly selected candidate |

Rectangle fill, flood fill, random scatter, copy/paste groups and multi-region editing are later conveniences. Add them after basic selection and write safety are stable. Group a continuous brush stroke into one understandable undo action rather than hundreds of accidental history entries.

## 4. Shared commands and editor state

The GUI owns selection, camera position and temporary tool state. The command layer owns content changes. A drag operation builds a candidate command; releasing it submits the command through the same service used by the CLI. The GUI does not write raw JSON itself.

Keep editor preferences separate from authored gameplay files. Grid visibility, panel widths and recent-file history should not create map diffs. Optional collaborative annotations should have an explicit model and review policy if introduced later; they should not be hidden in arbitrary unknown fields.

Undo records must include sufficient before/after data to restore terrain, identity, placement and related references. Undo after an external modification is a new guarded transaction and can conflict. An in-memory undo stack does not provide crash recovery; autosaved drafts and session recovery are separate features.

## 5. Validation and error experience

Errors should explain the consequence and the location: “The bank has no reachable interaction tile from the player spawn” is more useful than “Invalid map.” Each diagnostic needs a stable code, severity, region/placement reference and optional tile range. Selecting a diagnostic should focus the relevant location without discarding the current edit.

Separate structural failure from unfinished design. Broken JSON or duplicate IDs cannot safely enter the model. A structurally valid draft with an unfinished route can be saved and recovered while clearly marked as failing gameplay validation. Published content must pass its required validation profile. Warnings should not be silently converted into errors; choose and document which conditions block a particular release.

Do not offer an automatic “fix all” that moves NPCs, removes buildings or changes rewards without showing exact proposed changes. Useful quick fixes can still exist, such as focusing a missing reference or suggesting a valid approach tile, but they submit ordinary reviewable commands.

## 6. Draft recovery and content publication

The proposed state model is:

1. **Draft:** editable, recoverable work. Some gameplay checks may fail.
2. **Validated:** the exact semantic hash passed a defined validator version and profile.
3. **Published:** an immutable content revision is available for explicit loading by the game.

Editing a validated draft returns it to Draft. Publication selects the exact validated bytes, not whichever file now occupies the same path. A validation result that refers to an earlier hash cannot authorise a later change.

Autosaves should preserve a small documented recovery history in the editor workspace, using transactional writes. On reopening after interruption, show the original revision and the recovered candidate with a diff. Recovering must be a deliberate choice and must never overwrite the active published content or a player save as a side effect.

**Current bounded recovery:** The [implemented recovery workflow](EDITOR_RECOVERY.md) journals accepted runtime-valid edits, previews a C++-validated candidate and restores to a fresh file. It handles interrupted writes and changed/missing originals without overwriting them. Recovery resets undo history while preserving authored ID reservations. The [publication workflow](WORLD_PUBLICATION.md) validates exact saved bytes, creates named fixed world/manifest pairs and selects a version for a fresh game process. Broader package/release activation and structurally valid but gameplay-invalid drafts remain proposed. The game's [periodic player autosave](../06-development/PLAYER_AUTOSAVE_2026-11-20.md) remains separate.

## 7. Human accessibility and discoverability

Use readable scaling, keyboard navigation and visible focus indicators. Every colour-coded state also needs a label, icon shape or textual explanation. Provide coordinate entry as an alternative to dragging, and a searchable placement list as an alternative to finding a tiny object on a large map. Preserve commonly used actions when panels are resized.

Keyboard shortcuts should be discoverable in menus and remappable where the chosen UI toolkit makes that practical. Destructive actions need clear scope and undo support; confirmation dialogs are most useful for abandoning unsaved work or an irreversible publication step, not every reversible tile edit.

Native screen-reader support depends on the selected GUI toolkit and platform integration. Record that choice explicitly instead of promising accessibility that an SDL drawing surface alone cannot supply. Machine-readable CLI output is valuable for automation but does not replace a usable human interface.

## 8. Assets and content editors

Asset import should begin with a stable asset ID, source file, dimensions, anchor, visual bounds and provenance. Asset failure must be diagnosable and retain a debug rendering fallback. Artwork must not secretly define gameplay collision. A tall tree sprite can overhang the road while its actual occupied tile remains clear in an overlay.

Dialogue, quest, item, recipe and economy editors should first expose the current catalogue structure. Each then gains editing through a tested adapter. Do not introduce a general scripting system to avoid defining clear content types. New runtime behaviours require engine work, even if an editor can display an attractive field for them.

For economy previews, show assumptions and seed/sample size. A simulated result is a design estimate, not evidence of actual player behaviour. For quests, show unresolved references, unreachable required objectives and reward conditions before publication.

## 9. Completion criteria

Alpha 0.1 is complete when a human and a headless agent can perform the same bounded map change, obtain equivalent canonical content and diagnostics, recover an interrupted draft, detect an external edit, and launch the selected test content without changing normal player state. A preview alone is insufficient.

Later workspaces are complete one at a time when one real definition can be authored, validated, loaded and exercised through its existing authoritative gameplay path. Keep the [CLI contract](CLI_AND_AI_CONTRACT.md), [Editor workflows](EDITOR_WORKFLOWS.md) and [Master roadmap](../02-roadmap/MASTER_ROADMAP.md) aligned as each capability becomes available.
