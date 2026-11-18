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

**Local verification actually performed:**

- `g++ -std=c++20 -Wall -Wextra -Werror -pedantic -fsyntax-only src/Tools/WorldEditSession.cpp` passed with manually reconstructed minimal interface headers matching the required API signatures.
- `g++ -std=c++20 -O2 -Wall -Wextra -Werror -pedantic src/Tools/WorldEditSession.cpp stubs.cpp -o session_test` passed, linking **mock** snapshot, history, validator and draft-file functions.
- The mock-backed command session exercised `list`, `map 30 30`, move to `(30, 30)`, `status`, `undo`, `redo`, rejected water placement, `save`, `quit`. Exit status was 0, source input remained byte-identical and an output draft was created.
- The exact GitHub branch diff was inspected and the changed backlog JSON parsed successfully.

**Scope of evidence:** These checks validate basic tool syntax and command dispatch only. The execution environment cannot resolve `github.com` to clone the full repository, so the genuine production source/core, real snapshot fixture and CMake integration tests **have not** run. The new session is not verified for gameplay parity, Windows, concurrency or client rendering. The pull request must stay draft until full verification.

**Next gate:** Run a complete exact-branch headless build and the starter-world test
family, including this new interactive session test, then use the same shared typed
service for an SDL/Tk desktop grid UI with mouse selection and hotkeys.
