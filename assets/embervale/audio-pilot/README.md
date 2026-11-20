# Action sound pilot

Fifteen original mono, 48 kHz, signed 16-bit little-endian PCM WAV effects for
the current client. This is a first sound pass; audible quality, Windows device
acceptance and the missing local melee animation integration remain open.

| Cue | Files | Trigger |
|---|---|---|
| Wood contact | wood-1.wav, wood-2.wav | Authored gathering contact pose |
| Stone contact | stone-1.wav, stone-2.wav | Authored Mining contact pose |
| Swing | swing-1.wav | Local melee action starts |
| Heavy swing | heavy-swing-1.wav | Local godsword melee action starts |
| Hit | hit-1.wav, hit-2.wav | Committed positive melee damage involving the player |
| Block | block-1.wav | Committed hit with zero damage |
| Miss | miss-1.wav | Committed miss |
| Gather | gather-1.wav | Committed local item and XP reward |
| Craft | craft-1.wav | Recipe production successfully commits |
| Equip | equip-1.wav | Accepted local equipment action |
| Inventory | inventory-1.wav | Accepted drop or crafting inventory interaction |
| Eat | eat-1.wav | Accepted local food use |

The [generator](../../../tools/audio/build_audio_pilot.py) is the editable source:
frequency mixtures, envelopes, deterministic cosmetic seeds and sample conversion
are recorded there. Run `python tools/audio/build_audio_pilot.py` from the
repository to regenerate, or add `--check` to validate exact reproducibility
without writing files. The [manifest](manifest.json) records format, duration,
frame count, peak, RMS, provenance and SHA-256 for every clip.

All inputs are mathematical sine/noise synthesis created for Adzffs/GameEngine
with AI assistance. No sampled recordings or third-party sound assets were used.
No separate licence is granted by this batch; the repository's distribution
policy applies.

Each clip has a short attack/release, zero endpoints and at most 0.28 full-scale
peak. Playback uses six reusable streams with 0.45 maximum voice gain, per-cue
minimum intervals and cosmetic round-robin variants. Overload drops sounds
instead of building a delayed queue. Cosmetic choice never consumes gameplay RNG.

Settings provides effects volume and mute. F8 toggles mute; F9/F10 change volume
by ten percentage points. Controls apply to the current session. Muting/zero
volume clears queued stream data and discards new cues until sound is enabled.
Unavailable devices or missing/invalid optional clips leave gameplay usable.
There are no music or ambient controls in this pilot.
