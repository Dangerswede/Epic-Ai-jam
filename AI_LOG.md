# AI log

How AI tools were used during development.

## Tools
- Claude Code (code generation, planning, repo setup)
- Unreal MCP (editor automation from Claude Code)

## Log
| Date | Tool | What | Notes |
|------|------|------|-------|
| 2026-11-11 | Claude Code | Repo setup: .gitignore, Git LFS config, doc templates | Before jam, tooling only |
| 2026-11-12 | Claude Code | T0: CLAUDE.md, ARetakeCharacter (spring-arm camera, Enhanced Input move/look/jump/interact/Cut on R), ARetakeGameMode as default game mode | Jam content; build verified |
| 2026-11-12 | Claude Code | T1: FTakeFrame/FTakeData, URetakeRecorderComponent (fixed 30 Hz, action bitflags), OnActionPerformed on character, Retake.DumpTake console command | Build verified |
| 2026-11-12 | Claude Code | T2: ATakeGhost (30 Hz interpolated replay, action replay via shared Interact path, freeze at end, TAKE N label, dynamic ghost material), IRetakeInteractable, Retake.SpawnGhost debug command | Build verified (1 compile fix) |
| 2026-11-12 | Claude Code | T3: ARetakeSceneDirector state machine (Briefing/Rolling/Cut/SceneFailed/Wrap/DirectorsCut/Results), IRetakeResettable, built-in fallback Director's Cut replay | Build verified |
| 2026-11-12 | Claude Code | T4: ARetakeActivator base, APressurePlate, ALever, AGate, AGoalZone, AKillVolume (engine basic shapes, swappable mesh slots) | Build verified |
| 2026-11-12 | Claude Code | T5: Scene01_Bridge graybox level, M_Ghost material and BP_TakeGhost generated via editor Python script (Scripts/build_scene01.py); default map set | Level contents verified by script |
