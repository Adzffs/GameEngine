# STUDIO-024 terminal editor bridge — 18 November 2026

**Branch:** `studio/interactive-world-session` stacked on PR #11 (`studio/placement-undo-redo`).

**Implementation:** Bounded interactive `EmbervaleWorldMove --session` opens a validated
starter snapshot, displays a character-grid map and stable-ID list, supports in-memory
resource/workstation moves with shared inverse history, and saves only a validated new
sibling draft through `PublishNew` using the exact input-file SHA-256. It does not
change the runtime, player saves, file format, or independently reimplement move rules.
No external editor changes are automatically rebased.

**Regression evidence:** `StarterWorldInteractiveSessionTests.cmake` exercises the
real CLI entrypoint, map/list display, move, undo/redo, invalid destination, guarded
output, unchanged source, full runtime validation of new draft, and prevention of
existing-draft overwrite. These integration checks require a complete project build
and are **not yet run**.

**Local verification limit:** The execution environment cannot clone the complete
repository (GitHub DNS resolution fails). The branch was reviewed against exact parent
sources using the connected GitHub service, but production compilation and integration
tests have not yet been performed. Do not merge before running them on a complete checkout.

**Next gate:** Run a complete exact-branch headless build and the starter-world test
family, including this new interactive session test, then use the same shared typed
service for an SDL/Tk desktop grid UI with mouse selection and hotkeys.
