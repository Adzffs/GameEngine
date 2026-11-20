# Fixed world versions

`river-crossing-v1` publishes the existing [River Crossing draft](../worlds/river-crossing.blockout.json) without changing its geography. Its `manifest.json` records the exact source/canonical world SHA-256, world schema and validation implementation revision. Published names are create-only; keep these bytes together.

From the repository root, with the SDL client and content tools built:

```bash
build-client/EmbervaleWorldPublish --inspect content/published/river-crossing-v1/manifest.json
build-client/GameEngine --world-version content/published/river-crossing-v1/manifest.json --no-save --profile fresh
```

Use the executable paths appropriate to your platform (`.exe` on Windows). An older validator revision is inspected and revalidated with the current supported runtime profile. A changed checksum or invalid map fails startup.

In Studio, press **Versions…**, choose this directory as the version library, select a version and press **Play selected version**. To roll back a future launch, select an older version. Player progress is temporary for Studio launches. This library has no automatic “current” pointer and does not replace normal game startup.

See [world publication](../../docs/embervale/04-studio/WORLD_PUBLICATION.md) for authoring, interruption policy, metadata and scope. In-place map overwrite, player-state rollback, general asset packages and online deployment are separate work.
