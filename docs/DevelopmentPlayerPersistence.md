# Development Player persistence

The development application opts into Player persistence at
`saves/development-player.save`, relative to the process working directory.
This path is application policy: `World` neither knows the path nor controls
startup or shutdown timing. Command-line and production configuration are
future work.

`EngineConfiguration::developmentPlayerSavePath` controls the policy. Its
default `std::nullopt` value preserves the nonpersistent Engine path; supplying
a complete path opts in. Runtime entity IDs are assigned afresh by the World
and are never stored as persistent identities.

Startup loads an existing Player or creates the development-default Player
before gameplay. A missing file is not written during startup, and the file
store creates its parent directory only when an actual save occurs. Controlled
shutdown saves the tracked runtime Player through the existing atomic file
store. Startup does not rewrite loaded data, and ticks do not autosave; callers
may explicitly use `SaveNow` at a future policy boundary.

This guarantees structured startup/shutdown failure reporting and a save on a
successful controlled shutdown. It does not guarantee saving after forced
termination, an operating-system crash, or power loss; periodic autosave,
signal-safe persistence, and physical-media flushing beyond the existing file
store guarantees are also out of scope. Startup and controlled-shutdown
failures propagate through `Engine::Run()` to the process exit status. This
lifecycle is not crash-proof.
