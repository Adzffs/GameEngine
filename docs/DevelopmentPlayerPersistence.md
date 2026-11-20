# Development Player persistence

**Current code:** [fa5e0e9](https://github.com/Adzffs/GameEngine/commit/fa5e0e93efa71ac60519287cfd9a35b328ec6270), 20 November 2026. See the [autosave policy and verification](embervale/06-development/PLAYER_AUTOSAVE_2026-11-20.md) and [save compatibility guide](embervale/03-engine/SAVE_COMPATIBILITY.md). Save format 13 is unchanged.

The normal development application selects saves/development-player.save relative to its working directory. Imported-world and fresh-profile launches are temporary by default; --no-save stays temporary. An explicit --save path opts into the same lifecycle. Existing launch-path checks prevent a save or sidecar from replacing the selected world file. See [launch examples](embervale/06-development/GETTING_STARTED.md).

EngineConfiguration::developmentPlayerSavePath controls this application policy. Its default nullopt preserves the nonpersistent Engine path. World neither selects a path nor starts a timer. Runtime entity IDs are assigned afresh and are not persistent identities.

Startup loads an existing Player or creates a new one according to the selected profile. Missing files are not written immediately; directories are created only when a save actually occurs. Corrupt startup fails without enabling autosave or erasing recovery artifacts.

After each completed World tick, Engine polls WorldPlayerPersistenceLifecycle. The first steady-clock deadline is 60 seconds after successful startup. A successful periodic save or SaveNow restarts that interval. Long gaps and catch-up ticks produce one current snapshot, not a burst of historical writes. A failed save retains the active lifecycle and retries after 10 seconds.

A due save waits while the player is dead and runs on the first committed living/respawn tick. Crafting cursor/grid items are projected into saved inventory without live mutation or closing the session. Insufficient capacity rejects before writing and preserves the old save and live items.

Successful periodic saves show a brief “Progress saved” notice. A nonmodal storage/crafting warning remains visible until a retry succeeds. Controlled shutdown still saves current state and then disarms autosave. Failed shutdown retains the existing return-to-game or explicit unsaved-exit choices.

The synchronous file store uses bounded text, temporary/backup files and transactional replacement. It is not a physical-media durability guarantee. Forced termination restores the last completed checkpoint; later progress can be lost. Normally that is roughly a minute, but slow ticks/I/O, death or failures can extend the gap. No power-loss, OS-crash or signal-safe write guarantee is added. A killed-process test verifies reload after a completed autosave; it does not kill inside a filesystem operation.

Lifecycle startup and controlled-shutdown failures remain structured and propagate through Engine::Run to process status. The [dated report](embervale/06-development/PLAYER_AUTOSAVE_2026-11-20.md) records the policy, tests, code boundaries and remaining Windows/human acceptance.
