# Bug report template

## Summary

State the observable problem and who it affects. Include the planning/issue ID and whether it is a crash, incorrect gameplay, content validation, save compatibility, tooling or documentation problem.

## Environment and exact inputs

- Commit/branch and build configuration:
- Operating system, compiler, CMake and SDL versions:
- Content package/hash and schema version, if applicable:
- Player profile type: fresh, development boosted or restored save:
- Relevant seed/tick state and starting resources:
- Reproduction frequency and smallest known fixture:

## Reproduction

1. Establish the recorded starting state.
2. List the smallest actions that trigger the behaviour.
3. Record actual output and expected output.

Attach sanitised diagnostics, logs, screenshot/preview or fixture where useful. For an economy issue, list every purchase, conversion, sale, leftover item and resulting balance. For a position issue, give region, coordinates, surface and approach direction.

## Evidence and impact

Distinguish observed behaviour from a source-derived hypothesis. Record affected saves/content, reproducible test result and any regression range. Do not label an unknown CI job failure as a particular unit-test failure.

## Resolution

Record cause, implementation PR, regression scenario, verification results and remaining limitations. Link updated documentation where the old instructions were wrong.
