# Controls and save recovery

**Updated:** 20 November 2026.  
**Status:** Implemented and Windows regression verified in [4454067](https://github.com/Adzffs/GameEngine/commit/44540678bfa682870f2c9efb8907a1d36891910a). Wider keyboard/scale/manual acceptance remains open.\
[Candidate and verification handoff](../06-development/USABILITY_REVIEW_2026-11-20.md).

| Context | Control | Behavior |
|---|---|---|
| World | Left click | Move or interact |
| World | Right click | Resolve the current object and open its actions |
| World | R | Toggle walking/running when gameplay owns the keyboard |
| World/stations outside bank | H | Open help |
| Help | Enter, Space, Escape, H or click | Dismiss the overlay without a world action |
| Dialogue | Tab / Shift-Tab, Down / Up | Select next / previous control; > identifies focus |
| Dialogue | Enter / Space | Activate selection; initial focus prefers Continue/Close or an authored choice |
| Dialogue | Escape | Queue authoritative close, or dismiss terminal text locally |
| Bank | Text / Backspace | Filter / delete search; h is literal text |
| Panels | Escape | Close the top help/context menu, dialogue/shop/station, or side panel |
| Anywhere | Ctrl+Q or window close | Attempt normal guarded shutdown |
| Audio | F8, F9 / F10 | Mute, lower / raise effects volume |
| Art comparison | F6 | Toggle current world art |

Dialogue stays locked while its command awaits the server; **Waiting for server...**
explains the 600-ms tick delay. Rejection unlocks the same node safely. Escape
preserves queued/in-flight crafting work; shop closure respects its in-flight guard.
Queued station mouse-close and bank Escape cases now pass the [Windows verification](../06-development/WINDOWS_USABILITY_2026-11-20.md).

Losing focus cancels an unfinished crafting drag/context menu. It does not pause
the authoritative world or discard accepted commands. Inventory, crafting and
bank item selection still need wider keyboard access; fonts remain fixed-size.

Fresh/imported-world persistent play requires an explicit save. Autosave normally
checkpoints every 60 seconds and retries failures every ten seconds. Failure text
stays visible until success, positioned above open bank/station/shop/help panels.

If shutdown cannot save, **Return to game** is the Enter/Escape default. Restore
the writable save location or finish crafting/free bag space, then retry.
**Exit without saving** discards changes since the last successful checkpoint;
the process still reports unsuccessful final saving. Temporary no-save playtests
do not create player saves.
