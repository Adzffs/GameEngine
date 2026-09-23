# GameEngine documentation

This folder documents the game direction, systems, content, tests and development plan. Keep durable design knowledge here beside the code. Use GitHub Issues or a GitHub Project for changing task status; link each task to the relevant design brief and implementation PR.

## Start here

- [Game direction](game-direction.md)
- [Game design document](game-design.md)
- [Current status and repository snapshot](current-status.md)
- [Development roadmap](roadmap.md)
- [Development workflow](development-workflow.md)
- [System index](systems/README.md)
- [Content catalogue plan](content/README.md)
- [Verification strategy](testing/verification-strategy.md)
- [Open design decisions](decisions/README.md)
- [Templates](templates/)
- [Task tracking approach](tracking/README.md)

## Documentation rules

- Distinguish **implemented**, **design target**, **proposal**, and **needs verification**. A source file or commit title proves presence, not behavior or completion.
- Give systems and content stable IDs. Never reuse an ID.
- Put the task’s changing status in GitHub Issues/Projects. Keep specifications and lasting decisions in Markdown here.
- Every feature issue should link to a short feature brief and state acceptance evidence.
- Update these files in the same branch/PR as changes that alter their behavior.

## Starting snapshot

This documentation bootstrap reflects repository state checked on **2026-09-23**, `master` at `aa7525150621c688efdcb4a013d0863b51f7c73a`. GitHub Actions run #53 failed on both required jobs. Refresh [current status](current-status.md) before treating the snapshot as current.
