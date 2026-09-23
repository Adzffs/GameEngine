# System Index

This index is a navigation map, not a claim that every system is complete. The workbook Systems sheet contains the current evidence notes and next audit action.

| Area | Current foundation or target | Status rule |
|---|---|---|
| World and simulation | Deterministic tick, world authority and seeded randomness | Verify ordering and current tests. |
| Movement and actions | Eight-direction movement; start/cancel/complete action lifecycle | Verify edge cases and player-facing feel. |
| Gathering and mining | Resource actions, successful awards, Mining Basics quest | Close authoritative lifecycle and RNG evidence gaps. |
| Inventory and equipment | Inventory slots, transactions, equipment-derived stats | Audit full inventory and integration behavior. |
| Crafting and recipes | Recipe actions and smithing foundation; ingredient-grid crafting is a major target | Do not treat existing recipe action as grid crafting. |
| Combat and enemies | Melee, retaliation, death/respawn and AI foundations | Define combat feel, enemy variety and encounter pacing. |
| Quests | Generic quest framework, Gathering Basics and Mining Basics in current master history | Verify persistence, World lifecycle and command rejection evidence. |
| Dialogue and NPCs | Friendly NPC interaction and server-owned dialogue sessions | Use to guide real content; expand only against needs. |
| Shops and economy | Server-authoritative buying/selling and trade UI | Audit command lock and establish economy sinks/balance. |
| Persistence | Versioned player save and lifecycle integration | Maintain historical schema rules and add new state safely. |
| Skills | Skills source area exists; game-facing roster/progression is not settled | Inspect code, decide progression and test it. |
| Pets | Core design pillar; implementation not established in available evidence | Choose role/acquisition and implement one representative pet. |
| Bosses and loot | Core design pillar; implementation not established in available evidence | Specify first repeatable encounter and fair loot structure. |
| Client UI | SDL client with dialogue/shop/quest presentation foundations | Audit onboarding, feedback and slice usability. |
| Build and CI | Linux headless and Windows UCRT64 workflow exists | Current run #53 failed both jobs; diagnose. |
| World/content | First representative authored region not established in available evidence | Design compact first region and progression route. |

Create a dedicated system specification from `07 Templates/System Specification.md` when a system has distinct invariants, data, APIs, persistence, authority, failure behavior or tests that contributors need to implement it safely.
