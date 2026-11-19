# Local River Crossing launch correction

**Date:** 19 November 2026\
**Base:** master `fdb30e442719353e63bb43edfe734843329d2ca2` (River Crossing, PR #28).

## Cause

The earlier manual command assumed an updated checkout, the matching executable
in `build-client`, a repository-root working directory and the runtime DLL path.
The inspected older local copy still had `int main()` without argument parsing
and an older executable in that build directory. Supplying the new map/profile
flags cannot update an old game binary or source tree.

## Correction

`Play-RiverCrossing.cmd` resolves the source/map paths relative to itself, checks
for current source and UCRT64 dependencies, and configures a dedicated
`build-river-crossing` directory with explicit GNU compiler/Ninja/SDL paths.
It builds only the game with tests and standalone tools disabled, then launches
the selected world using `--profile fresh --no-save`. Its DLL path is local to
that launcher process. A toolchain installed elsewhere can be selected through
`GAMEENGINE_UCRT64_BIN`. Build/startup errors remain visible in the console.

The launcher does not pull/switch Git, reuse an arbitrary older executable,
replace another checkout or read/write the development player save. The
`--build-only` option builds without opening a window.

## Observed result

- The launcher was invoked from outside the repository with `--build-only`.
  A new Windows UCRT64 configuration built all 95 required game/core steps,
  with test and graphics-test targets disabled; exit 0.
- The resulting binary was started with the current River Crossing snapshot,
  fresh profile and no-save arguments, using the matching DLL path.
- The game window was observed directly. It displayed the arrival road, Mara,
  the bank/Elia area, walkable building pads, river and northern crossing. The
  tutorial was at the guide step, with full health and stamina.
- The game was left open for the user. No test suite was run.

This closes the immediate local-startup issue for the updated working copy.
It is a narrow visible-launch observation, not completion of the deferred
combat, quest, AI, save/reload or starter-pacing acceptance work.
