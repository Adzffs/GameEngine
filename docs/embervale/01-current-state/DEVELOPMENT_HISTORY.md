# Development history

**Recorded:** 18 November 2026.  
**Default-branch baseline:** `e9d32932e8f9b27e96283313cca320058792a81f`.  
**Purpose:** Preserve the project’s delivered sequence and give future contributors a reliable route back to the original changes.

## 1. What this history means

The history below is derived from the current `master` commit listing and selected source verification. The complete appendix contains the 234 unique commits returned across three pages, beginning with the initial engine setup and ending with the game-feel merge. Dates in the appendix are converted to Europe/London and explicitly show BST or GMT.

A commit title describes a recorded change. It is not a claim that the feature is finished, that the design is final, or that current tests pass. Use [Implemented Systems](IMPLEMENTED_SYSTEMS.md) for the current behaviour inventory and [Evidence Ledger](EVIDENCE_LEDGER.md) for verification boundaries.

## 2. Project chapters

### July 2026: engine and playable-system foundations

The earliest sequence establishes the engine, fixed tick, entities and their manager, movement requests, tile-grid rules, world objects, resources, and action lifecycle. Subsequent changes connect click-to-move pathfinding, basic world rendering, inventory rewards, equipment, and resource depletion/respawn. The project moves from isolated engine structures to a playable gathering loop. Representative evidence: [initial setup](https://github.com/Adzffs/GameEngine/commit/97bca770ec126b270196cf523a8dac8cf1b20a21), [world grid](https://github.com/Adzffs/GameEngine/commit/33a05b5856897f1407f6aa5437eadc81dcb939c9), [click-to-move](https://github.com/Adzffs/GameEngine/commit/62c81ea8d65b4f34dd71f44f4b82fdb4ec895378), and [inventory rewards](https://github.com/Adzffs/GameEngine/commit/0eb5e30ab68db47cddb40d48c0a8852603f4f347).

Mining and smithing then expand the resource loop with tool requirements, resource tiers, recipes, crafting stations, a furnace, timed production, and repeatable actions. The reusable action framework acquires explicit states and cancellation reasons. These systems remain the basis for the current resource-to-equipment chain. Representative evidence: [Mining skill](https://github.com/Adzffs/GameEngine/commit/f0d39f1e36b02b7714fbf6db2ab524a48ef902c2), [recipe database](https://github.com/Adzffs/GameEngine/commit/4e58ea41234b4cca74446258fb2ec8af54f40087), and [gathering action loop](https://github.com/Adzffs/GameEngine/commit/d96ad005f70b94e367b71c481332afbb8a889441).

### July 2026: combat, authority, and persistence

Combat work adds health and ratings, equipment-derived statistics, deterministic attack resolution, a combat service, timed melee, monster targeting, repeated engagements, retaliation, deaths, rewards, and respawn. Later commits add status effects, healing, aggression, and startup content validation. Representative evidence: [combat service](https://github.com/Adzffs/GameEngine/commit/a8fcc98e09ce0a433a580c15740a8c4469765bb3), [reward tables](https://github.com/Adzffs/GameEngine/commit/fee9b3f6736f1062003f2deb1be0cb5df133fec7), and [monster respawn](https://github.com/Adzffs/GameEngine/commit/321e3460e8c59f70c422dce8c7bdd46f0fd64b35).

The architecture then separates SDL from the headless core, centralises tick ownership, introduces independent randomness seams, indexes entity lookup, adds a server-command boundary and tick scheduler, and extracts several systems from World. These changes create the seams that the Studio plan should reuse. Representative evidence: [headless split](https://github.com/Adzffs/GameEngine/commit/582e2519327a8f7650fbd5f666fb9f85e2e69f3a), [command boundary](https://github.com/Adzffs/GameEngine/commit/25fd9a0949c97bca4d544ca2139cd04ab924831c), and [scheduler](https://github.com/Adzffs/GameEngine/commit/ee0ba6f559d5541f5d5d905d5712254010a848fa).

NPC definitions and spawns, wandering, friendly conversation, dialogue graphs and choices, shops, versioned player data, a deterministic text codec, atomic file storage, and World persistence follow. These are existing foundations; replacing them wholesale is unnecessary for the editor milestone. Representative evidence: [NPC/spawn foundation](https://github.com/Adzffs/GameEngine/commit/d2ae8c7d2d9dcac9346aae407f9a205bd1a88e3b), [dialogue graph](https://github.com/Adzffs/GameEngine/commit/bb25bd57747f900b2121e4b368a8472a8771e8c4), and [save storage](https://github.com/Adzffs/GameEngine/commit/334a7722c472d5f40caf2bb2e71ca1fc8e743142).

### September 2026: client integration, CI, and quests

September connects persistence, dialogue, and shop systems into the client, improves session/gating feedback, and adds Linux and Windows workflow configuration. Gathering Basics and Mining Basics establish quest vertical slices; subsequent work generalises quest state and persistence and improves UI layouts and migration validation. Representative evidence: [client persistence](https://github.com/Adzffs/GameEngine/commit/40f435a784522260a403ec8afac57d929159b75d), [CI foundation](https://github.com/Adzffs/GameEngine/commit/1b9f8323490912f1360cce15cda582d133c89514), [Gathering Basics](https://github.com/Adzffs/GameEngine/commit/82ff19ad6baed0410371549916eeae6c2cbb4346), and [September baseline](https://github.com/Adzffs/GameEngine/commit/aa7525150621c688efdcb4a013d0863b51f7c73a).

The separate documentation proposal in [PR #1](https://github.com/Adzffs/GameEngine/pull/1) dates its snapshot to this period. It was still open and unmerged at the 18 November review. Its game direction and templates remain useful; its progress snapshot should not be treated as the current state.

### Early November 2026: authoritative grid crafting

The next major group adds the crafting table, atomic inventory stack operations, deterministic recipe metadata, and authoritative crafting sessions. It creates the inventory/grid/cursor transaction foundation for Minecraft-inspired crafting interactions. Representative evidence: [crafting table](https://github.com/Adzffs/GameEngine/commit/12a88535478040f896479df2ab4c2d4d1f3b53e6), [stack operations](https://github.com/Adzffs/GameEngine/commit/49801e79c0b261a3cb75c50d9a79ee6bffaf1ec3), and [crafting sessions](https://github.com/Adzffs/GameEngine/commit/f14cc5d0a6636dd455f78233f1bd1aa9a3aa8202).

### 16–17 November 2026: connected game-feel and content work

This branch delivers a large sequence: grid-crafting client interaction, onboarding and inventory presentation, hover/context feedback, ground items, bank storage and sessions, a bank window, tutorial persistence, progression feedback, a more readable development region, camera/minimap work, combat styles, death recovery and protected recovery piles, additional enemies, crafting upgrades, recipe discovery and book presentation, economy modelling, named characters, investigation/crafting/combat quests, journal guidance, and opening/combat feedback. Representative evidence: [game-feel start](https://github.com/Adzffs/GameEngine/commit/a63ba512421ed381e7adfe332ef0292dc0050ed6), [bank sessions](https://github.com/Adzffs/GameEngine/commit/32ad50addb193001c87c1a2eb98e63001427c33b), [tutorial persistence](https://github.com/Adzffs/GameEngine/commit/cf3437259d15dec8921f7355db3b6b3d0dfbf384), and [full cast](https://github.com/Adzffs/GameEngine/commit/ade60e45ebfc69cecba1d1aa424fa6d4ebe1ab52).

The final portion concentrates on running, pathing, zoom, click transforms, route changes, and L-shaped running. [PR #2](https://github.com/Adzffs/GameEngine/pull/2) merges this branch into `master`. Its description reports 81 passing tests, while the resulting exact-head push workflow later reports failure without accessible step logs in this review. Preserve both records and resolve the current verification gap. [L-shaped running](https://github.com/Adzffs/GameEngine/commit/92ca8c8f6529bb1f190d4b7f48ce82fff58dd948) · [merge](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f) · [current run](https://github.com/Adzffs/GameEngine/actions/runs/44018694678).

## 3. Work that is documented but not yet implemented in this baseline

The starter-region discussions establish a preference for a balanced, organic geography with several connected activity areas and no central core. River Crossing, terrain contours, placement rules, and future elevation belong to the design direction. The Studio plans describe shared content definitions, a map editor, a headless command interface, asset tooling, additional content editors, validation, recovery, and AI usability. The reviewed game tree does not contain those tools yet.

Planning work is real work and belongs in project history. Its deliverable is a decision, specification, acceptance criterion, or roadmap item. It should not be entered in the implementation tracker as an already functioning editor or shipped region.

## 4. Corrections the new roadmap must retain

| Earlier ambiguity | Current documented treatment |
|---|---|
| An illustrative small map could be mistaken for the current world | Actual development map is 100×100; export the real baseline before designing a replacement. |
| Test-count or PR language could imply a fresh green build | 81 is the CMake target count; current remote run is failed; fresh evidence remains required. |
| Editor and elevation planning could be mistaken for implemented systems | Explicitly mark Studio and elevation as planned. |
| Development defaults could be used to measure a beginner journey | Establish a genuine fresh-player profile; current configuration injects development XP. |
| A previous document could be discarded because it is old | Preserve useful policy/design intent and reconcile stale factual sections. |
| Every requested tool could expand Alpha 0.1 | Keep the first vertical slice bounded; later editors, pets, complex bosses, and online services have separate gates. |

The baseline facts in this table are sourced in [Implemented Systems](IMPLEMENTED_SYSTEMS.md) and [Evidence Ledger](EVIDENCE_LEDGER.md).

## 5. Complete recorded commit inventory

The following list is chronological by returned history order reversed to oldest first. Commit dates are author timestamps shown in Europe/London; merged branches can therefore contain timestamps interleaved with work from other branches. Titles are copied from the first line of each commit message. Use the linked commit for its actual patch and parentage.

| Date and time (London) | Commit | Recorded change |
|---|---|---|
| 2026-07-15 16:28 BST | [97bca77](https://github.com/Adzffs/GameEngine/commit/97bca770ec126b270196cf523a8dac8cf1b20a21) | Initial game engine setup |
| 2026-07-15 16:38 BST | [957c820](https://github.com/Adzffs/GameEngine/commit/957c820e976d53a251ff6783d9690c7d6210faf7) | Added fixed server tick system |
| 2026-07-15 16:50 BST | [a2a1a28](https://github.com/Adzffs/GameEngine/commit/a2a1a2871f03ff5688821369c31b1d5d9888a5ec) | Added gitignore and removed build files |
| 2026-07-15 17:00 BST | [b5aaa4b](https://github.com/Adzffs/GameEngine/commit/b5aaa4bdeb53fe3bbf585f212de2af72d841d4be) | Added entity system foundation |
| 2026-07-15 17:07 BST | [2432181](https://github.com/Adzffs/GameEngine/commit/2432181d8bb03c1e1532208e49e97cb2b3a415fc) | Added entity manager system |
| 2026-07-15 17:25 BST | [0b34521](https://github.com/Adzffs/GameEngine/commit/0b34521bceb6c4dd825c8123aca956ddd402286d) | Added entity types |
| 2026-07-15 19:27 BST | [02cbfa0](https://github.com/Adzffs/GameEngine/commit/02cbfa0d15940f44d21a053b0b82c1086a38ee11) | Added entity movement system |
| 2026-07-15 19:38 BST | [33a05b5](https://github.com/Adzffs/GameEngine/commit/33a05b5856897f1407f6aa5437eadc81dcb939c9) | Added tile based world grid |
| 2026-07-15 20:09 BST | [1151508](https://github.com/Adzffs/GameEngine/commit/11515088fa6653e876986d027aa45ef257760705) | Added tile types |
| 2026-07-15 20:24 BST | [2e0fc8d](https://github.com/Adzffs/GameEngine/commit/2e0fc8d5f6e1a0ec07f04a0ce661b98f746c4665) | Connected entity manager to world map |
| 2026-07-15 20:30 BST | [59f1b2c](https://github.com/Adzffs/GameEngine/commit/59f1b2c02ac37a61ed7957cc51134399c365405c) | Made movement map aware |
| 2026-07-15 20:39 BST | [2897839](https://github.com/Adzffs/GameEngine/commit/2897839e32e128aa84e982578ded5d5198a4b8b1) | Added terrain movement rules |
| 2026-07-15 20:43 BST | [14c3201](https://github.com/Adzffs/GameEngine/commit/14c3201190f8cf172231d414bdf1b9cb2d019d8a) | Added world object base class |
| 2026-07-15 20:48 BST | [9e327b1](https://github.com/Adzffs/GameEngine/commit/9e327b1bb0dd4f2ca0a5346175da0935ab7cfae2) | Added resource node system |
| 2026-07-15 20:58 BST | [88109d9](https://github.com/Adzffs/GameEngine/commit/88109d9d96a315939efc8e1e1b77ee8c2602a37a) | Added world object manager |
| 2026-07-15 21:41 BST | [4f7e562](https://github.com/Adzffs/GameEngine/commit/4f7e56248ccfa7103b28aac1fc72dc19e0769e7d) | Cleaned project files and removed test artifacts |
| 2026-07-15 22:06 BST | [b8ed4ff](https://github.com/Adzffs/GameEngine/commit/b8ed4ff5911baa109e5c33ef964384d32558708c) | Added action lifecycle management |
| 2026-07-16 00:14 BST | [e400550](https://github.com/Adzffs/GameEngine/commit/e4005505a542deacbb6b8289be2111ce794e4465) | Added movement request and input manager foundation |
| 2026-07-16 00:22 BST | [ba640ac](https://github.com/Adzffs/GameEngine/commit/ba640ac36c1f4e3f3a166af1ab412f159f1138de) | Added movement request input pipeline |
| 2026-07-16 01:17 BST | [b16301c](https://github.com/Adzffs/GameEngine/commit/b16301cb20e990db7d9fffbddef4003a7767e064) | Refactored entity system using polymorphism |
| 2026-07-16 13:26 BST | [3924856](https://github.com/Adzffs/GameEngine/commit/3924856f50a3e93479bc68f5f9ea8dc7d3898f89) | Make Entity an abstract base class |
| 2026-07-16 14:13 BST | [d682bb6](https://github.com/Adzffs/GameEngine/commit/d682bb636d99b6f45bc9d2853ea7c4fea77e6a34) | Add server authoritative movement request queue |
| 2026-07-16 14:30 BST | [f398e31](https://github.com/Adzffs/GameEngine/commit/f398e31b8f3574b9cf038ae7ada8fe4ac46c640e) | Move player input handling into Engine |
| 2026-07-16 14:44 BST | [efbb610](https://github.com/Adzffs/GameEngine/commit/efbb610bbf6e5f4dbaa32ab9c75baa4b148b2a43) | Add NPC entity subclass |
| 2026-07-16 15:53 BST | [ab561c1](https://github.com/Adzffs/GameEngine/commit/ab561c16d5b33604bbb298ac03ccb611bdfae94e) | Render player from world position |
| 2026-07-16 16:56 BST | [62c81ea](https://github.com/Adzffs/GameEngine/commit/62c81ea8d65b4f34dd71f44f4b82fdb4ec895378) | Add click to move pathfinding |
| 2026-07-16 17:02 BST | [bc0fd68](https://github.com/Adzffs/GameEngine/commit/bc0fd6868b3aea868f3bb95af1cfd8a604918489) | Render map collision tiles |
| 2026-07-16 17:21 BST | [01d9dab](https://github.com/Adzffs/GameEngine/commit/01d9dab7486a39eeb7f9acfe7dcb57d8fe35bbe4) | Render NPCs and resource nodes |
| 2026-07-16 17:44 BST | [f510760](https://github.com/Adzffs/GameEngine/commit/f510760b57ba90264029e8e74fece03aa6a6dcd8) | Add nearest reachable path fallback |
| 2026-07-16 18:38 BST | [3605d9f](https://github.com/Adzffs/GameEngine/commit/3605d9f4f51b37ed2d4f0e66fcdbf0981836ffe0) | Add resource interaction targeting |
| 2026-07-16 19:08 BST | [d4917bd](https://github.com/Adzffs/GameEngine/commit/d4917bd2d52954e8286bc09be3e2beb480f9c81e) | Add resource depletion and respawn cycle |
| 2026-07-16 19:21 BST | [0eb5e30](https://github.com/Adzffs/GameEngine/commit/0eb5e30ab68db47cddb40d48c0a8852603f4f347) | Add player inventory and resource rewards |
| 2026-07-16 19:31 BST | [9c1dd1d](https://github.com/Adzffs/GameEngine/commit/9c1dd1d7b9ba94fc485daea1dbc35a41ec5bf393) | Display player inventory in game UI |
| 2026-07-16 19:48 BST | [89a084f](https://github.com/Adzffs/GameEngine/commit/89a084fc526eb19d5d3b3a3b903fd5246d760277) | Add slot based inventory interface |
| 2026-07-16 20:29 BST | [953b715](https://github.com/Adzffs/GameEngine/commit/953b7156fb5b6ab5727e99cc5d93f64fe053eeb0) | Add tabbed side panel interface |
| 2026-07-16 21:03 BST | [defb3b8](https://github.com/Adzffs/GameEngine/commit/defb3b8f52b07c08d2d117fd556d31381bbd8334) | Clean up engine logging |
| 2026-07-16 21:26 BST | [b66a706](https://github.com/Adzffs/GameEngine/commit/b66a706c8a58a0ac5c0dcbb544821a1264b79602) | Add item definitions and inventory stack rules |
| 2026-07-16 22:37 BST | [0dc1d69](https://github.com/Adzffs/GameEngine/commit/0dc1d69061e099383a7c2139423b4e2e4ef5e204) | Add bronze axe and woodcutting tool requirement |
| 2026-07-16 23:42 BST | [7e04440](https://github.com/Adzffs/GameEngine/commit/7e044404d053529cfb1154592caf661ac616a564) | Add interactive equipment system |
| 2026-07-17 00:11 BST | [60ef490](https://github.com/Adzffs/GameEngine/commit/60ef4901a1a1e2178a67a3572530280e49f2286c) | Add axe tiers and equipment swapping |
| 2026-07-17 00:23 BST | [cc73c4d](https://github.com/Adzffs/GameEngine/commit/cc73c4d8f3d31d7af8bdbbb06f20eaa3ce4c003c) | Add axe level requirements |
| 2026-07-17 01:07 BST | [4b15534](https://github.com/Adzffs/GameEngine/commit/4b1553402b66bdc75a7ec35b5bdd6a0e38703691) | Add multiple tree types and resource definitions |
| 2026-07-17 16:14 BST | [25faa49](https://github.com/Adzffs/GameEngine/commit/25faa492766776baee2b9853f5e3049b75e7873a) | Add maximum uses to resource definitions |
| 2026-07-17 16:29 BST | [2224257](https://github.com/Adzffs/GameEngine/commit/2224257dfad676b52b4ab845879fb33d47053cea) | Add multiple uses to resource nodes |
| 2026-07-17 16:44 BST | [8f7f107](https://github.com/Adzffs/GameEngine/commit/8f7f1071be54c6a635f35ca4af8dc51d076a6a63) | Add continuous resource gathering |
| 2026-07-17 16:55 BST | [2caf738](https://github.com/Adzffs/GameEngine/commit/2caf738ec215eef03526df484b74715f65c075cc) | Add immediate action cancellation |
| 2026-07-17 17:02 BST | [2ec88b1](https://github.com/Adzffs/GameEngine/commit/2ec88b12d7d726446c984cf80570f208a564db9e) | Add development player configuration |
| 2026-07-17 17:31 BST | [981c6cc](https://github.com/Adzffs/GameEngine/commit/981c6cc87424ae495a41d13646106e3dda67cdaa) | Add resource gathering success chances |
| 2026-07-17 17:56 BST | [d6f8a3e](https://github.com/Adzffs/GameEngine/commit/d6f8a3ed43cef4eb9121dc15e569e4291501985e) | Add required tool type to resource definitions |
| 2026-07-17 19:12 BST | [e228aee](https://github.com/Adzffs/GameEngine/commit/e228aee95af0c0817d24cc1f3f3f1a9a4d6c425e) | Generalise resource gathering requirements |
| 2026-07-17 19:22 BST | [f0d39f1](https://github.com/Adzffs/GameEngine/commit/f0d39f1e36b02b7714fbf6db2ab524a48ef902c2) | Add Mining skill |
| 2026-07-17 19:26 BST | [b49a2ab](https://github.com/Adzffs/GameEngine/commit/b49a2ab09cce1b5d84d2167c38e09a5276190b3f) | Add Bronze pickaxe |
| 2026-07-17 19:31 BST | [7816475](https://github.com/Adzffs/GameEngine/commit/78164759604eca2ddb2409410ed11f3154bef315) | Add Bronze pickaxe graphics |
| 2026-07-17 19:36 BST | [2b9af54](https://github.com/Adzffs/GameEngine/commit/2b9af5480640710f35d1455d4a3e7fa41b5a5f1c) | Add Copper mining resource data |
| 2026-07-17 19:40 BST | [c705f2a](https://github.com/Adzffs/GameEngine/commit/c705f2ac6f3906ba5a3f6fc6b5ace20ee348efe4) | Add Copper rock to the world |
| 2026-07-17 19:53 BST | [5a35b2c](https://github.com/Adzffs/GameEngine/commit/5a35b2c7ff043b46e484d01da5b7b59dc9346def) | Generalise skills panel layout |
| 2026-07-17 19:55 BST | [9fef6b7](https://github.com/Adzffs/GameEngine/commit/9fef6b7a929993f021860dc58d5257652c07ef72) | Add Tin mining resource data |
| 2026-07-17 20:09 BST | [6e7a1e9](https://github.com/Adzffs/GameEngine/commit/6e7a1e9f96ff7c5df1a62cdeb10b0dc1ca43789f) | Add resource-specific depleted visuals |
| 2026-07-17 20:15 BST | [2ce30d5](https://github.com/Adzffs/GameEngine/commit/2ce30d5766fe7f4bd43311b49bc674ec9c779fca) | Add Mining pickaxe progression |
| 2026-07-17 20:20 BST | [a3bb33a](https://github.com/Adzffs/GameEngine/commit/a3bb33a5954901a27e6f7d28b8f7e72540c5c41b) | Add Iron mining resource |
| 2026-07-17 20:24 BST | [97caca9](https://github.com/Adzffs/GameEngine/commit/97caca9a12e74e522940732ca8d77517810c5ca5) | Add Smithing skill and bar item types |
| 2026-07-17 20:28 BST | [e8d384d](https://github.com/Adzffs/GameEngine/commit/e8d384d10a320c111fb160fdb8563beb4941fedb) | Add recipe system foundation |
| 2026-07-17 20:33 BST | [4e58ea4](https://github.com/Adzffs/GameEngine/commit/4e58ea41234b4cca74446258fb2ec8af54f40087) | Implement recipe database |
| 2026-07-17 20:39 BST | [2775128](https://github.com/Adzffs/GameEngine/commit/277512892756e48b0db53f722420d334b32831a4) | Add recipe validation |
| 2026-07-17 20:43 BST | [72d82db](https://github.com/Adzffs/GameEngine/commit/72d82dbaaf980d5855184ef305b7286a297f741c) | Add recipe execution |
| 2026-07-17 20:48 BST | [b3a7025](https://github.com/Adzffs/GameEngine/commit/b3a702549fe1bd09f9f9b5656000c679cd425282) | Add bar item graphics |
| 2026-07-17 20:54 BST | [a70fc57](https://github.com/Adzffs/GameEngine/commit/a70fc57439d8163d4d0029d2a39ad4e7a9d31e55) | Add crafting station foundation |
| 2026-07-17 21:07 BST | [88365b9](https://github.com/Adzffs/GameEngine/commit/88365b9ebe176f1a634f5108718ea80c163da2b9) | Add furnace interaction menu |
| 2026-07-17 21:15 BST | [cd53d3a](https://github.com/Adzffs/GameEngine/commit/cd53d3ad79324f028cd5f1c8ddee73d1f5714f66) | Add clickable furnace recipes |
| 2026-07-17 21:24 BST | [b6c5a70](https://github.com/Adzffs/GameEngine/commit/b6c5a70fcb3bd61d4392bfd8394ae37238403ba4) | Add timed smelting actions |
| 2026-07-17 21:31 BST | [f8c1943](https://github.com/Adzffs/GameEngine/commit/f8c1943fae19dbc3560ed64810e24e9666c12827) | Add continuous smelting |
| 2026-07-17 22:00 BST | [9ac5b0f](https://github.com/Adzffs/GameEngine/commit/9ac5b0f116798e3ad3c1816803a1ddc2535c4b70) | Add action progress display |
| 2026-07-17 22:06 BST | [8cbeff4](https://github.com/Adzffs/GameEngine/commit/8cbeff4df7f6dde1570d7ec7883ac970b4e7b367) | Validate recipe crafting stations |
| 2026-07-17 22:20 BST | [5b25e5a](https://github.com/Adzffs/GameEngine/commit/5b25e5ae1da252b32ed7cc45bd6e222a4be9404b) | Add steel bar recipe |
| 2026-07-18 17:34 BST | [4e49b19](https://github.com/Adzffs/GameEngine/commit/4e49b19e36032f49f59451c61b9672564bb5248e) | Add action lifecycle states |
| 2026-07-18 17:41 BST | [298f48d](https://github.com/Adzffs/GameEngine/commit/298f48d8e1120fdccb0c267e15c6ffbc20918dd0) | Centralize action lifecycle management |
| 2026-07-18 17:45 BST | [c8c4f3a](https://github.com/Adzffs/GameEngine/commit/c8c4f3a10250cd39a9eae397f149b4cfd1abd7ca) | Centralize action lifecycle management |
| 2026-07-18 18:24 BST | [380a7b2](https://github.com/Adzffs/GameEngine/commit/380a7b2706770d6dbaa2ea6c138de9ebf5ac1b4a) | Centralize gathering start validation |
| 2026-07-18 18:34 BST | [816067e](https://github.com/Adzffs/GameEngine/commit/816067e916ff7e5765436fcc4c475bfec6756643) | Centralize gathering validation |
| 2026-07-18 18:41 BST | [d96ad00](https://github.com/Adzffs/GameEngine/commit/d96ad005f70b94e367b71c481332afbb8a889441) | Move gathering onto reusable action loop |
| 2026-07-18 18:46 BST | [e56eea7](https://github.com/Adzffs/GameEngine/commit/e56eea72f1612e88cc8ebb75386edcd7bb68f434) | Add explicit action cancellation reasons |
| 2026-07-18 18:50 BST | [8028105](https://github.com/Adzffs/GameEngine/commit/8028105ab062275a2f664c4a9a5f87db4439fb37) | Add explicit action cancellation reasons |
| 2026-07-18 18:59 BST | [ea472d7](https://github.com/Adzffs/GameEngine/commit/ea472d7d9291afd4b2979aafa6ab4763df868516) | Cancel gathering when equipped tool changes |
| 2026-07-18 19:11 BST | [ad6f778](https://github.com/Adzffs/GameEngine/commit/ad6f77811b049a25d2e0dbc080e07c160d677ef0) | Centralize smithing validation |
| 2026-07-18 19:18 BST | [41b3df9](https://github.com/Adzffs/GameEngine/commit/41b3df934211c1a6eac2167cf4e9d01bcd69a620) | Move smithing onto reusable action loop |
| 2026-07-18 19:29 BST | [efc246b](https://github.com/Adzffs/GameEngine/commit/efc246ba4096a64efd50606fb2be9069c984aa48) | Harden reusable action framework |
| 2026-07-18 19:40 BST | [30f84dc](https://github.com/Adzffs/GameEngine/commit/30f84dc06ea59dfd4c65c9181f80250d885ecef5) | Add equipment-derived player stats |
| 2026-07-18 19:50 BST | [f504208](https://github.com/Adzffs/GameEngine/commit/f504208d9b30ddc6da6f3e5c8f37d9bb4088f00b) | Add player health and combat ratings |
| 2026-07-18 20:03 BST | [5e1a9fd](https://github.com/Adzffs/GameEngine/commit/5e1a9fd9a170257a21d3989f7aacfb877f175c58) | Add deterministic melee attack resolution |
| 2026-07-18 20:10 BST | [a8fcc98](https://github.com/Adzffs/GameEngine/commit/a8fcc98e09ce0a433a580c15740a8c4469765bb3) | Add server-owned combat service |
| 2026-07-18 20:16 BST | [77d0b0a](https://github.com/Adzffs/GameEngine/commit/77d0b0a5c6eb81ed1aa776c285710e98998256c9) | Add timed melee combat actions |
| 2026-07-18 20:25 BST | [b83235c](https://github.com/Adzffs/GameEngine/commit/b83235c0c9568baa00d7788b727c7ccef8966478) | Generalize melee combat with passive monsters |
| 2026-07-18 20:57 BST | [eea7925](https://github.com/Adzffs/GameEngine/commit/eea79251334942308be54f54857fb42835f437b9) | Add repeating melee engagements |
| 2026-07-18 21:05 BST | [2564f24](https://github.com/Adzffs/GameEngine/commit/2564f243a543f48294203f78cf6cca10e8352ea3) | Add pending melee interaction requests |
| 2026-07-18 21:17 BST | [f9d60c6](https://github.com/Adzffs/GameEngine/commit/f9d60c6201bbd29e1a7bfc4d3d093068a3c89d4e) | Add monster rendering and hit testing |
| 2026-07-18 21:28 BST | [e34e4bd](https://github.com/Adzffs/GameEngine/commit/e34e4bdf6ae50df9aba116f12d2e1b2f940f395e) | Connect monster clicks to melee interactions |
| 2026-07-18 21:48 BST | [2c1c036](https://github.com/Adzffs/GameEngine/commit/2c1c036903115106335cda673232888ecbf8c5fc) | Add passive development monster spawn |
| 2026-07-18 22:04 BST | [db16c75](https://github.com/Adzffs/GameEngine/commit/db16c75a387217ca97ba67b4fb97c78cc80856c9) | Add monster combat feedback |
| 2026-07-18 22:19 BST | [ca3a356](https://github.com/Adzffs/GameEngine/commit/ca3a35657d7e75026cc1fd0bedcacd1009464d3b) | Add developer godsword for combat testing |
| 2026-07-18 22:36 BST | [ac262a8](https://github.com/Adzffs/GameEngine/commit/ac262a871dc23b99ccb8468f23752fbeaeefc62e) | Add monster retaliation and player death handling |
| 2026-07-18 22:57 BST | [f6c1490](https://github.com/Adzffs/GameEngine/commit/f6c1490718a530ed8888a99d3146f42dac803f1f) | Add entity death events and kill credit |
| 2026-07-18 23:07 BST | [fee9b3f](https://github.com/Adzffs/GameEngine/commit/fee9b3f6736f1062003f2deb1be0cb5df133fec7) | Add deterministic reward table foundation |
| 2026-07-18 23:27 BST | [183fd98](https://github.com/Adzffs/GameEngine/commit/183fd9871b088618d17b341e140816ea01c0f5d5) | Grant monster loot from death events |
| 2026-07-18 23:59 BST | [321e346](https://github.com/Adzffs/GameEngine/commit/321e3460e8c59f70c422dce8c7bdd46f0fd64b35) | Add deterministic monster respawning |
| 2026-07-19 00:02 BST | [c4396eb](https://github.com/Adzffs/GameEngine/commit/c4396ebf257c57889c4849356404c0c9a9223c0b) | Fix material item stackability |
| 2026-07-19 00:40 BST | [fcd148a](https://github.com/Adzffs/GameEngine/commit/fcd148ab0488054f857420b980090c8088ffc0ff) | Add universal requirement framework |
| 2026-07-19 00:50 BST | [5441c22](https://github.com/Adzffs/GameEngine/commit/5441c2200f7eb4e0af56c5965a2263626f6790cd) | Add generic equipment unequip flow |
| 2026-07-19 01:05 BST | [d16ef39](https://github.com/Adzffs/GameEngine/commit/d16ef39ad8967bf5e9184c68ccf268cc7334208b) | Add action lifecycle events |
| 2026-07-19 01:21 BST | [3fc8b7d](https://github.com/Adzffs/GameEngine/commit/3fc8b7da8f0e29c4cff6b50a9ef987242bd8b3ae) | Add startup content validation |
| 2026-07-19 01:31 BST | [ceab708](https://github.com/Adzffs/GameEngine/commit/ceab7083de5bbdae2e6c971dd06f71f848014a3f) | Add timed status effect foundation |
| 2026-07-19 01:39 BST | [3f2b2e8](https://github.com/Adzffs/GameEngine/commit/3f2b2e8a29658a2b21c4b91ec2e4933e359855dd) | Add timed status effect foundation |
| 2026-07-19 01:55 BST | [df6671b](https://github.com/Adzffs/GameEngine/commit/df6671b44faf3ca6c9747fb4a2ff688b7cf86701) | Add basic food healing |
| 2026-07-19 02:13 BST | [160d296](https://github.com/Adzffs/GameEngine/commit/160d296f03f29223a8838f89d334385988b88cba) | Add aggressive monster behaviour |
| 2026-07-19 02:47 BST | [582e251](https://github.com/Adzffs/GameEngine/commit/582e2519327a8f7650fbd5f666fb9f85e2e69f3a) | Decouple SDL from headless core |
| 2026-07-19 03:00 BST | [8e81ae7](https://github.com/Adzffs/GameEngine/commit/8e81ae7317d5a3288a72b9bdaea5dbca4e29e35a) | Centralize gameplay tick ownership |
| 2026-07-19 03:09 BST | [0083e6a](https://github.com/Adzffs/GameEngine/commit/0083e6af01c6764bad5eb3d5ea72e06ee56a7d45) | Fix server tick pacing and overrun tracking |
| 2026-07-19 03:58 BST | [9f0d438](https://github.com/Adzffs/GameEngine/commit/9f0d43816795493e5855b667bdd08f4838c08326) | Inject deterministic gathering randomness |
| 2026-07-19 04:32 BST | [cd08660](https://github.com/Adzffs/GameEngine/commit/cd0866020c2e651803889bd6d4974795a7528738) | Harden movement path execution |
| 2026-07-19 04:41 BST | [8ad716e](https://github.com/Adzffs/GameEngine/commit/8ad716e095c1bdb5bae76d1504fda4b0e9301fa6) | Index entity lookup by ID |
| 2026-07-19 04:48 BST | [a5fffac](https://github.com/Adzffs/GameEngine/commit/a5fffac5c49e16a15125a1f5a00c419084ec18a8) | Make aggressive monster combat test deterministic |
| 2026-07-19 05:05 BST | [25fd9a0](https://github.com/Adzffs/GameEngine/commit/25fd9a0949c97bca4d544ca2139cd04ab924831c) | Add server command boundary |
| 2026-07-19 05:15 BST | [ee0ba6f](https://github.com/Adzffs/GameEngine/commit/ee0ba6f559d5541f5d5d905d5712254010a848fa) | Add deterministic tick scheduler |
| 2026-07-19 05:27 BST | [2ed757e](https://github.com/Adzffs/GameEngine/commit/2ed757e0540c9a3f3ca1e29a6afa2cdd82f5c185) | Extract monster AI decisions from World |
| 2026-07-19 05:43 BST | [bcafeb1](https://github.com/Adzffs/GameEngine/commit/bcafeb165a287c8fce21f8a7e10686a3263c244f) | Extract movement path orchestration from World |
| 2026-07-19 05:53 BST | [6fcd873](https://github.com/Adzffs/GameEngine/commit/6fcd873299cd85d04c8a9be661165cd9696c44ac) | Extract melee engagement orchestration from World |
| 2026-07-19 06:06 BST | [897f71c](https://github.com/Adzffs/GameEngine/commit/897f71cf04a8c2f5e76c88b295dedc8c0b6988d2) | Extract world object interaction orchestration |
| 2026-07-19 12:59 BST | [e124aac](https://github.com/Adzffs/GameEngine/commit/e124aacd75037538070cc29d06c0aac581af5efe) | Extract gathering action completion from World |
| 2026-07-19 13:38 BST | [b29c257](https://github.com/Adzffs/GameEngine/commit/b29c25738c85ac708f9d60f603c24f7b02266b4b) | Extract recipe action completion from World |
| 2026-07-19 14:14 BST | [83f6e87](https://github.com/Adzffs/GameEngine/commit/83f6e87aaca8bd4c105e3201cd06e24231e58564) | Separate melee evaluation from damage application |
| 2026-07-19 14:29 BST | [de0c22a](https://github.com/Adzffs/GameEngine/commit/de0c22a2e9397599b510b96e5f79ece53f894313) | Simplify melee completion context handling |
| 2026-07-19 15:02 BST | [d2ae8c7](https://github.com/Adzffs/GameEngine/commit/d2ae8c7d2d9dcac9346aae407f9a205bd1a88e3b) | Add NPC definition and spawn foundations |
| 2026-07-19 16:17 BST | [c9bd4f4](https://github.com/Adzffs/GameEngine/commit/c9bd4f4d3677b313bda3fd2be3753428b1646b54) | Add runtime NPC spawn management and wandering |
| 2026-07-19 17:08 BST | [0bfbd02](https://github.com/Adzffs/GameEngine/commit/0bfbd02cce6efbe175711ef5aa8efeb481361713) | Add friendly NPC talk interaction foundation |
| 2026-07-19 17:45 BST | [bb25bd5](https://github.com/Adzffs/GameEngine/commit/bb25bd57747f900b2121e4b368a8472a8771e8c4) | Add server-authoritative dialogue graph foundation |
| 2026-07-19 18:16 BST | [0b28136](https://github.com/Adzffs/GameEngine/commit/0b2813635bf6602ceea961bc30e76430786e8fa2) | Add server-authoritative dialogue choices and branching |
| 2026-07-19 18:58 BST | [392dbfc](https://github.com/Adzffs/GameEngine/commit/392dbfc5bb0734efdeb056996a1b13fefa631245) | Add server-authoritative NPC shop foundation |
| 2026-07-19 20:00 BST | [704a1ba](https://github.com/Adzffs/GameEngine/commit/704a1ba8420f9a6f4e781d7615e7b05fafd3599b) | Add versioned player save-state foundation |
| 2026-07-19 20:37 BST | [6d4cf98](https://github.com/Adzffs/GameEngine/commit/6d4cf980e66c36dcab5b926c3b2c946181027b92) | Add deterministic player save text codec |
| 2026-07-19 21:04 BST | [334a772](https://github.com/Adzffs/GameEngine/commit/334a7722c472d5f40caf2bb2e71ca1fc8e743142) | Add atomic local player save storage |
| 2026-07-19 22:01 BST | [82e5229](https://github.com/Adzffs/GameEngine/commit/82e52290aff7589c44cda6a57b243acc840fba97) | Add manual World player persistence integration |
| 2026-09-06 01:44 BST | [40f435a](https://github.com/Adzffs/GameEngine/commit/40f435a784522260a403ec8afac57d929159b75d) | Integrate player persistence into client lifecycle |
| 2026-09-06 02:48 BST | [423bef6](https://github.com/Adzffs/GameEngine/commit/423bef6f70831cb163b1c1a3676c01ff80424142) | Integrate NPC dialogue into client |
| 2026-09-06 02:56 BST | [1b9f832](https://github.com/Adzffs/GameEngine/commit/1b9f8323490912f1360cce15cda582d133c89514) | Add Linux and Windows build verification |
| 2026-09-06 02:59 BST | [95a2f31](https://github.com/Adzffs/GameEngine/commit/95a2f3111c1729763cf6bca56864a15c424debc7) | Fix CI workflow expression syntax |
| 2026-09-06 03:01 BST | [6c300ac](https://github.com/Adzffs/GameEngine/commit/6c300ac7be51430e94e0059c995c55a34ba198d0) | Fix CI trigger definition |
| 2026-09-06 03:22 BST | [a7dad32](https://github.com/Adzffs/GameEngine/commit/a7dad32e1011dda75f6b28b96fd80dd823473be1) | Add manual CI workflow dispatch |
| 2026-09-06 03:53 BST | [e691d9e](https://github.com/Adzffs/GameEngine/commit/e691d9e834ef84c9318c71ffdb700197a9509eb8) | Integrate NPC shop into client |
| 2026-09-06 04:09 BST | [dd7fe6c](https://github.com/Adzffs/GameEngine/commit/dd7fe6c8d9cf967905a935002374fa942ca9046f) | Present NPC trade as dialogue choice |
| 2026-09-06 04:17 BST | [e83899b](https://github.com/Adzffs/GameEngine/commit/e83899b56f89ef947fef2306fa280b0e24bb1429) | Gate trade by dialogue service node |
| 2026-09-06 04:23 BST | [ee37aeb](https://github.com/Adzffs/GameEngine/commit/ee37aeba23d187c774f898f9b8ad506c6e016543) | Require matching runtime dialogue node for trade |
| 2026-09-06 04:27 BST | [3ba1a97](https://github.com/Adzffs/GameEngine/commit/3ba1a97fb7cea17efa91d9b6c6f99bef7b1e26d8) | Preserve dialogue presentation across ticks |
| 2026-09-06 04:29 BST | [9c9a0c3](https://github.com/Adzffs/GameEngine/commit/9c9a0c365b905943bbad19490222f1aa83611317) | Author trade as dialogue service action |
| 2026-09-06 04:38 BST | [2504bf0](https://github.com/Adzffs/GameEngine/commit/2504bf02d94011bf1cd0039b9c74e13c96ae06ef) | Show shop transaction rejection feedback |
| 2026-09-06 04:41 BST | [8edd845](https://github.com/Adzffs/GameEngine/commit/8edd8451a42f5a61e10d518c6e1e7efc4d9b1b2b) | Clear shop rejection feedback on synchronization |
| 2026-09-06 05:26 BST | [88347c0](https://github.com/Adzffs/GameEngine/commit/88347c037feda56f1a49647dbc82e917708d04e1) | Align shop controls with rendered hitboxes |
| 2026-09-06 05:35 BST | [3eaa87b](https://github.com/Adzffs/GameEngine/commit/3eaa87b7e87e2cf2ea31f84dff66a6c286ce007a) | Expose testable log shop offer |
| 2026-09-08 02:18 BST | [82ff19a](https://github.com/Adzffs/GameEngine/commit/82ff19ad6baed0410371549916eeae6c2cbb4346) | Add Gathering Basics quest vertical slice |
| 2026-09-08 03:03 BST | [beee074](https://github.com/Adzffs/GameEngine/commit/beee0743f0f7ec62937e839ec48e18e9883e6b5a) | Prevent overlapping client shop commands |
| 2026-09-23 04:27 BST | [4ea44e0](https://github.com/Adzffs/GameEngine/commit/4ea44e011fc5f39c6913b0df89f66eaeb2e90b83) | Generalize quest state and persistence |
| 2026-09-23 04:48 BST | [2119e9b](https://github.com/Adzffs/GameEngine/commit/2119e9b7133c98570fc5b34b1246b73700ff7ac9) | Fix generic quest client layouts |
| 2026-09-23 04:58 BST | [42e5dc2](https://github.com/Adzffs/GameEngine/commit/42e5dc217a1a9c3b143d4b1c8f9aa03fcb955df9) | Strengthen quest overflow clipping test |
| 2026-09-23 05:39 BST | [4f5d7dc](https://github.com/Adzffs/GameEngine/commit/4f5d7dcb58b256efdae607591a57b8000d91bc15) | Add Mining Basics quest |
| 2026-09-23 05:51 BST | [aa75251](https://github.com/Adzffs/GameEngine/commit/aa7525150621c688efdcb4a013d0863b51f7c73a) | Fix version 2 quest migration validation |
| 2026-11-03 07:08 GMT | [12a8853](https://github.com/Adzffs/GameEngine/commit/12a88535478040f896479df2ab4c2d4d1f3b53e6) | Add server-authoritative crafting table |
| 2026-11-03 07:51 GMT | [49801e7](https://github.com/Adzffs/GameEngine/commit/49801e79c0b261a3cb75c50d9a79ee6bffaf1ec3) | Add atomic inventory stack operations |
| 2026-11-03 08:09 GMT | [482889f](https://github.com/Adzffs/GameEngine/commit/482889ff52b4b9e6d90d1f444651d6d674116d49) | Add deterministic crafting recipe metadata |
| 2026-11-04 05:54 GMT | [f14cc5d](https://github.com/Adzffs/GameEngine/commit/f14cc5d0a6636dd455f78233f1bd1aa9a3aa8202) | Add authoritative crafting sessions |
| 2026-11-16 02:17 GMT | [ae575de](https://github.com/Adzffs/GameEngine/commit/ae575de514800e776863dff6af1576f74f8dfe91) | Add authoritative crafting interaction model |
| 2026-11-16 02:31 GMT | [d82de28](https://github.com/Adzffs/GameEngine/commit/d82de28c626178fd21db90ddd933b01910e94e74) | Connect Minecraft-style crafting interactions |
| 2026-11-16 02:58 GMT | [a63ba51](https://github.com/Adzffs/GameEngine/commit/a63ba512421ed381e7adfe332ef0292dc0050ed6) | Start game feel and interface overhaul |
| 2026-11-16 03:34 GMT | [39df38a](https://github.com/Adzffs/GameEngine/commit/39df38a1e057c48296b6e2513418d99fe52d2f6b) | Refine onboarding inventory and settings |
| 2026-11-16 03:38 GMT | [ad890dc](https://github.com/Adzffs/GameEngine/commit/ad890dcd34ccfc050132a01f11b01fd36108bedb) | Add deterministic world hover feedback |
| 2026-11-16 03:45 GMT | [11d23d2](https://github.com/Adzffs/GameEngine/commit/11d23d2b70508e24f870b8aef66687f7175ef5e1) | Add world context actions and examine feedback |
| 2026-11-16 03:50 GMT | [93db419](https://github.com/Adzffs/GameEngine/commit/93db419d9c6ccc8c6faadbfdde632df507514a3d) | Add authoritative ground item interactions |
| 2026-11-16 03:54 GMT | [1335e1c](https://github.com/Adzffs/GameEngine/commit/1335e1c9b441dc502057ad7d054e8a7b813ac7e0) | Add canonical player bank storage |
| 2026-11-16 04:01 GMT | [32ad50a](https://github.com/Adzffs/GameEngine/commit/32ad50addb193001c87c1a2eb98e63001427c33b) | Add secure persistent bank sessions |
| 2026-11-16 04:05 GMT | [dd67b44](https://github.com/Adzffs/GameEngine/commit/dd67b44db6a42a38d14c4b5506ac873d288d5c09) | Add interactive bank window |
| 2026-11-16 04:07 GMT | [b14c085](https://github.com/Adzffs/GameEngine/commit/b14c085ac3fac34b1b6baee0c5cf09df583d2321) | Add equipment comparison and preparation feedback |
| 2026-11-16 04:14 GMT | [cf34372](https://github.com/Adzffs/GameEngine/commit/cf3437259d15dec8921f7355db3b6b3d0dfbf384) | Add persistent guided tutorial progression |
| 2026-11-16 04:17 GMT | [858f854](https://github.com/Adzffs/GameEngine/commit/858f854ce1b9a944c132e271ca00215e0c0a2339) | Add rewarding progression feedback |
| 2026-11-16 23:08 GMT | [7bbc221](https://github.com/Adzffs/GameEngine/commit/7bbc22149bca9a98b09f18d27f0226708b0efe00) | Shape a readable starter region |
| 2026-11-16 23:18 GMT | [3791812](https://github.com/Adzffs/GameEngine/commit/3791812cb497d31b70a54f0b189526e3e20b52ba) | Add smooth camera and local minimap |
| 2026-11-16 23:23 GMT | [559c114](https://github.com/Adzffs/GameEngine/commit/559c1142bdd32ef263878ad03b5e6f3291ede7ef) | Improve combat target readability |
| 2026-11-16 23:28 GMT | [6d57234](https://github.com/Adzffs/GameEngine/commit/6d57234bdac48b04c860c7578458ea95736e73c8) | Add authoritative melee combat styles |
| 2026-11-16 23:50 GMT | [bc75d96](https://github.com/Adzffs/GameEngine/commit/bc75d965a4b52d29f7dbade21b8271f3f500f098) | Persist selected combat style |
| 2026-11-16 23:54 GMT | [f36db85](https://github.com/Adzffs/GameEngine/commit/f36db852c9c7480b6f2e7dfdd20b54f8afea4958) | Add safe player death recovery |
| 2026-11-16 23:56 GMT | [b5cace1](https://github.com/Adzffs/GameEngine/commit/b5cace16c3509ea0ef8e051e484175e92876cff4) | Explain player death and recovery |
| 2026-11-17 00:02 GMT | [0426170](https://github.com/Adzffs/GameEngine/commit/0426170bf8b1ae65e6311f15f9dd26f87f988801) | Add protected inventory recovery piles |
| 2026-11-17 00:08 GMT | [8fe87ff](https://github.com/Adzffs/GameEngine/commit/8fe87ffd8ef6a8887d1b97d701878ad5bc4b65ee) | Persist protected recovery piles |
| 2026-11-17 00:14 GMT | [4763741](https://github.com/Adzffs/GameEngine/commit/4763741bccea29cde5102c9b6aafcf701471124e) | Improve melee attack readability |
| 2026-11-17 00:29 GMT | [04dd6a4](https://github.com/Adzffs/GameEngine/commit/04dd6a4887dda2dfe0fc243c93cba7a9b568343f) | Define distinct enemy progression roles |
| 2026-11-17 00:38 GMT | [a142b3a](https://github.com/Adzffs/GameEngine/commit/a142b3aa6fcc62cd5c39556fadb4ad1cef737def) | Place advanced enemy encounters |
| 2026-11-17 00:41 GMT | [6334be4](https://github.com/Adzffs/GameEngine/commit/6334be49a65149413e5b0b445b03dd19eeca71d3) | Tie combat styles to progression |
| 2026-11-17 00:46 GMT | [9280539](https://github.com/Adzffs/GameEngine/commit/928053953deb30b54ad1b0f7be4bd35a29447f2b) | Define starter economy balancing ledger |
| 2026-11-17 00:50 GMT | [e00d33c](https://github.com/Adzffs/GameEngine/commit/e00d33cd393939803f824e3a84523325a5164690) | Forge metal equipment from gathered materials |
| 2026-11-17 00:53 GMT | [6fc1ece](https://github.com/Adzffs/GameEngine/commit/6fc1ece30555f5398a3204357465872e57d7ac8a) | Make advanced mining tools earned upgrades |
| 2026-11-17 00:57 GMT | [f6c0707](https://github.com/Adzffs/GameEngine/commit/f6c0707375c44ebb770a577e8dedd6a56e8c76a5) | Give higher timber tiers useful plank yields |
| 2026-11-17 01:08 GMT | [4de42e2](https://github.com/Adzffs/GameEngine/commit/4de42e270a634d917e89a2dc572c23910052a224) | Show recipe craftability and requirements |
| 2026-11-17 01:11 GMT | [8d530ca](https://github.com/Adzffs/GameEngine/commit/8d530cac65242ec0f199c9793c87e2d7f9992064) | Add authoritative player recipe books |
| 2026-11-17 01:16 GMT | [92d5057](https://github.com/Adzffs/GameEngine/commit/92d505725fe9bc59c4d4d7af774265d8ead2dc8a) | Persist recipe discoveries in save version 9 |
| 2026-11-17 01:22 GMT | [95c7b14](https://github.com/Adzffs/GameEngine/commit/95c7b14c052a873a92c4b05fbda4a150d0910fe3) | Connect recipe discovery to gameplay |
| 2026-11-17 01:24 GMT | [9c92530](https://github.com/Adzffs/GameEngine/commit/9c92530ddb1f1ed34c08aaba150823a364e25a63) | Add browsable recipe book panel |
| 2026-11-17 01:27 GMT | [dd36c7b](https://github.com/Adzffs/GameEngine/commit/dd36c7b0ae10048a6a72b90176b994b7e8f3784e) | Add detailed paged recipe book |
| 2026-11-17 01:30 GMT | [1959998](https://github.com/Adzffs/GameEngine/commit/1959998a4a5ea97835dd79bc47d636a24fcff044) | Speed up mastered recipe production |
| 2026-11-17 01:33 GMT | [f21bf72](https://github.com/Adzffs/GameEngine/commit/f21bf72ba643e125eba0cd28f66f22b71e76aba5) | Add premium healing tonic coin sink |
| 2026-11-17 01:36 GMT | [693a8d7](https://github.com/Adzffs/GameEngine/commit/693a8d7a100979ba45e9d6041e5dee39c2aa2e4c) | Clarify shop purchase decisions |
| 2026-11-17 01:37 GMT | [d3c7794](https://github.com/Adzffs/GameEngine/commit/d3c77941a259321a4b3306de7e0c6d88dc2038cd) | Add executable economy balance model |
| 2026-11-17 01:42 GMT | [ddefc8c](https://github.com/Adzffs/GameEngine/commit/ddefc8c378ef9361b839c5b7897a48ca222e7ae8) | Turn the guide into Embervale warden Mara |
| 2026-11-17 01:47 GMT | [d6577ea](https://github.com/Adzffs/GameEngine/commit/d6577eab0e8ea0058d4ce9dfc2f48eff358f323a) | Add Torren as Embervale workshop smith |
| 2026-11-17 01:52 GMT | [ade60e4](https://github.com/Adzffs/GameEngine/commit/ade60e45ebfc69cecba1d1aa424fa6d4ebe1ab52) | Complete Embervale starter character cast |
| 2026-11-17 01:56 GMT | [7d8f5cb](https://github.com/Adzffs/GameEngine/commit/7d8f5cbb2774247b7c00fb8e76a95810cd884b05) | Support character investigation objectives |
| 2026-11-17 02:06 GMT | [52d3bcf](https://github.com/Adzffs/GameEngine/commit/52d3bcf9b722993d4748cbaaf16943c0aeebd59b) | Add Tracks in the Ash investigation quest |
| 2026-11-17 02:11 GMT | [5e0b7d6](https://github.com/Adzffs/GameEngine/commit/5e0b7d61f7a39faca22dbb4c450dd40de09ad271) | Support crafted item quest objectives |
| 2026-11-17 02:15 GMT | [90184e4](https://github.com/Adzffs/GameEngine/commit/90184e45e4ac23bf0c0d28281aeee9dc34d32dab) | Support alternate quest resolutions |
| 2026-11-17 02:23 GMT | [6ed0776](https://github.com/Adzffs/GameEngine/commit/6ed07765e5e6ad27c08ab2ba66aaeb2de8310289) | Add Shield or Shelter crafting dilemma |
| 2026-11-17 02:27 GMT | [ef80be5](https://github.com/Adzffs/GameEngine/commit/ef80be551cf2f9dfcb95e8fb5b6cf88fdfc1d61e) | Support combat defeat quest objectives |
| 2026-11-17 02:38 GMT | [b7c530a](https://github.com/Adzffs/GameEngine/commit/b7c530aae1314541e347200a624305ba5b58204a) | Add Fangs at the Gate combat quest |
| 2026-11-17 02:49 GMT | [616fbab](https://github.com/Adzffs/GameEngine/commit/616fbaba83a5ab8cbcca485afd78f793b842b0d1) | Add Heart of the Ash region conclusion |
| 2026-11-17 02:54 GMT | [9a42c56](https://github.com/Adzffs/GameEngine/commit/9a42c56a526af65d5b2ee4695d18334e72fb674e) | Prioritize active quest journal goals |
| 2026-11-17 02:58 GMT | [f052735](https://github.com/Adzffs/GameEngine/commit/f052735c00018a73ee0c19b5d4384e12ea7d8b88) | Add actionable quest journal guidance |
| 2026-11-17 03:00 GMT | [ded32f5](https://github.com/Adzffs/GameEngine/commit/ded32f52f344f0998200e21aa1e7158ee727fb55) | Fit full starter arc in quest journal |
| 2026-11-17 03:10 GMT | [a26d189](https://github.com/Adzffs/GameEngine/commit/a26d189c9d61f8930192e8a4b56f423d11930db9) | Gate starter quests into a progression arc |
| 2026-11-17 03:12 GMT | [7bbfc74](https://github.com/Adzffs/GameEngine/commit/7bbfc74a29f66f4f765eb6a1f750e3a2faa96e2a) | Announce quest completion and unlocks |
| 2026-11-17 03:25 GMT | [64e3042](https://github.com/Adzffs/GameEngine/commit/64e3042cc51361bf2fdd5b60d80973c17f917538) | Add collapsible streamlined side panel |
| 2026-11-17 03:43 GMT | [ca51349](https://github.com/Adzffs/GameEngine/commit/ca51349e1d21451b9ab5297cf893401f1cf98833) | Improve camera and opening combat feedback |
| 2026-11-17 03:47 GMT | [b879656](https://github.com/Adzffs/GameEngine/commit/b879656bdfa36dc7b509b8161506ffaa9f13cccb) | Add stamina running and faster pathfinding |
| 2026-11-17 03:55 GMT | [7543c35](https://github.com/Adzffs/GameEngine/commit/7543c3592e1e12c26c3020d90cab3a8cf93d9cb5) | Fix zoom targeting and diagonal pathing |
| 2026-11-17 04:01 GMT | [2098e8c](https://github.com/Adzffs/GameEngine/commit/2098e8c1dbda369db72e0923a5825f751304fa1b) | Keep running movement on visible path segments |
| 2026-11-17 04:04 GMT | [d00ccd5](https://github.com/Adzffs/GameEngine/commit/d00ccd5ab761559e772dfe428b8590ab1569947f) | Harden running against rapid route changes |
| 2026-11-17 04:09 GMT | [b4b179b](https://github.com/Adzffs/GameEngine/commit/b4b179b6f597cd13dea6a478da28c92a23faab1a) | Preserve rapid run-toggle parity |
| 2026-11-17 04:12 GMT | [b903d9a](https://github.com/Adzffs/GameEngine/commit/b903d9a58a06cfd6ce55e7cdd2b5223ddc95e95a) | Lock zoom and click transforms per frame |
| 2026-11-17 04:15 GMT | [3980b9f](https://github.com/Adzffs/GameEngine/commit/3980b9f115601f19b452c0d251f9f5751fd97ebc) | Correct zoomed world viewport transform |
| 2026-11-17 04:21 GMT | [92ca8c8](https://github.com/Adzffs/GameEngine/commit/92ca8c8f6529bb1f190d4b7f48ce82fff58dd948) | Add OSRS-style L-shaped running |
| 2026-11-17 04:39 GMT | [e9d3293](https://github.com/Adzffs/GameEngine/commit/e9d32932e8f9b27e96283313cca320058792a81f) | Merge pull request #2 from Adzffs/wip/game-feel-overhaul |
