# Security, reliability and data protection

**Status:** Existing boundaries are source verified; the additional controls are implementation requirements or proposals, not a completed security audit.  
**Updated:** 18 November 2026  
**Scope:** Local game development, content import, Studio automation and future distribution.

## 1. Protect the things that matter

For the current project, the main assets are source/content history, player progression, trustworthy authored definitions and the ability to reproduce a working build. The immediate failure modes are malformed input, unintended writes, incompatible content, lost updates, unreachable saved state and misleading validation evidence.

An internet-facing account service, production server and multiplayer threat model are later work. The existing world authority is a valuable foundation, but it does not establish that networking, authentication or operational security already exists. Scope security work to the feature actually being built and expand it when new boundaries are introduced.

## 2. Current useful boundaries

| Boundary | Observed source behaviour | Limit of the evidence |
|---|---|---|
| Startup content checks | `main` invokes content validation before running the engine | Does not validate a future external map format that is not implemented |
| Player-save input | File store performs bounded reads and structured validation/error handling | Does not prove immunity to every filesystem/power-failure condition |
| World authority | Typed NPC/monster creation validates catalogue and spawn conditions | Not proof of a completed network-server boundary |
| Build separation | Headless core can build without SDL when both dependent options are OFF | Does not prove every target currently builds in this environment |
| CI permissions | Workflow requests read access to repository contents | Branch/ruleset enforcement is a separate setting |

Sources: [main.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/main.cpp), [PlayerSaveFileStore.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/Persistence/PlayerSaveFileStore.cpp), [World.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/World.cpp), [CMakeLists.txt](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/CMakeLists.txt), [CI workflow](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/.github/workflows/ci.yml).

## 3. External content must be bounded data

The proposed loader must validate encoding, schema version, file size, nesting depth, dimensions, arithmetic overflow, collection lengths, references and supported enum values before constructing a map or writing tiles. The current `Map::GetTile` directly indexes its vector; an external file must not be allowed to drive unchecked coordinates into it. This is a concrete integration requirement, not a claim that current compiled placements exploit that path. [Source: Map.cpp](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/src/World/Map.cpp).

Map and patch files should contain typed data, not shell commands, arbitrary scripts or plugin code. Reject unknown behaviour fields rather than executing or silently discarding them. Limit operations and affected cells in an automated batch so accidental requests cannot allocate unreasonable memory or freeze validation indefinitely.

Asset import introduces another parser boundary. Validate dimensions, atlas bounds, frame counts, supported formats and resource use; retain provenance. Imported artwork must not secretly change collision or grant behaviour that the runtime does not support.

## 4. Scope and protect filesystem writes

Studio commands operate on an explicit project root and target. Define how canonical paths and symlinks are handled, then enforce that policy before writes. Reject traversal outside the allowed scope. Output paths, recovery files and temporary files must follow the same policy as the primary content file.

For editing, compare the expected hash, build an in-memory candidate and recheck the base under a lock at commit time. Write temporary data in a suitable location and use the supported atomic-replacement strategy. A rename alone does not prevent two valid writers from overwriting each other, and a revision number alone is insufficient if content can change outside the editor.

Keep multi-file transactions explicit. Stage an immutable package and activate its manifest only after all required files are ready, or implement a journal with tested recovery. Never report an atomic package update when only individual files are atomic.

See [CLI contract](../04-studio/CLI_AND_AI_CONTRACT.md) for the complete proposed transaction algorithm.

## 5. Preserve player data and recovery paths

Player saves, editor drafts and published content have different ownership. A playtest uses an explicit test profile/save and cannot silently select a normal development save. Recovery drafts do not become active game content simply because they are newer.

The current persistence guide describes controlled startup/shutdown behaviour and explicitly does not guarantee save survival after forced termination, OS failure or power loss. Retain that honest durability boundary until stronger behaviour is implemented and tested. [Source: DevelopmentPlayerPersistence.md](https://github.com/Adzffs/GameEngine/blob/e9d32932e8f9b27e96283313cca320058792a81f/docs/DevelopmentPlayerPersistence.md).

Before a content migration, review every location-bearing state: player position, safe spawn, recovery pile, monster home/respawn and future region/surface references. Preserve items, XP, quest progress and ownership when relocating state. Unknown definitions need an explicit compatibility result, not silent deletion of the player's possessions.

Use synthetic save fixtures in tests. Keep personal saves and local machine paths out of public issue attachments and release packages. When a real save is needed for diagnosis, minimise and sanitise the reproduction rather than uploading unrelated player state.

## 6. Keep authority and automation aligned

Client/UI code should submit supported intentions and display results. Authoritative gameplay decides whether movement, interaction, rewards and state transitions are valid. When adding external content, preserve canonical catalogue checks through a coherent selected-content provider rather than removing checks to accept arbitrary coordinates.

AI editing uses the same typed commands, validation and conflict behaviour as human editing. A protocol wrapper should expose bounded inspect/dry-run/apply/validate operations, not unrestricted filesystem or shell access. If a command conflicts, the agent inspects and reconciles the new state; it does not bypass the expected hash.

Future multiplayer work must separately address identity, command validation, replay/duplicate messages, disconnects, rate/resource limits, persistent ownership and operational recovery. None of those controls should be declared implemented because a command queue already exists.

## 7. Dependencies, licences and reproducibility

Document new dependencies, supported versions, how they are obtained and the reason for adding them. Review update impact on the actual build and parser/rendering boundaries. Record the toolchain versions used for a release because floating package updates can change the environment even when source is unchanged.

The repository owner must choose a code licence and asset distribution policy. Keep third-party notices and asset provenance with the package. Do not infer permission from an image being downloadable or from the repository being public.

Do not commit credentials, private tokens, personal saves or generated caches. Repository ignore rules help prevent accidents but do not remove already tracked data; review the actual staged diff. If sensitive material is exposed, treat containment and credential replacement as an explicit incident rather than assuming a later deletion erases history.

## 8. Reporting and recovery

For an ordinary defect, report the source/content revision, environment, exact steps, expected/observed result and sanitised logs. If a finding could expose private data or enable abuse in a future deployed service, use the project's chosen private reporting route once established. This document does not invent a security email address or claim private vulnerability reporting is configured.

Triage a failure into input/content error, code defect, write conflict, persistence incompatibility, dependency/environment failure or unavailable evidence. Preserve the relevant prior state before attempting repair. Record the actual cause only after evidence supports it.

The inspected baseline's failed workflow has no usable execution/log evidence in this review; it is a reliability investigation item, not a proven vulnerability or proven test assertion failure. [Source: workflow run](https://github.com/Adzffs/GameEngine/actions/runs/44018694678).

## 9. Readiness checks by feature

Before the loader ships, prove bounded parsing, reference integrity and semantic parity. Before writes ship, prove conflict detection, interruption handling and recoverable drafts. Before a content package ships, prove exact-hash validation, required routes and save compatibility. Before a public game release, prove packaging, licence/provenance records and clean-environment operation. Before multiplayer exposure, define and test the additional network/account/operations boundaries.

These checks belong to their implementation milestones and [release gates](../02-roadmap/RELEASE_GATES.md). They are not a claim that the project has already passed a full security review.
