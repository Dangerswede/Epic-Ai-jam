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
| 2026-11-12 | Claude Code | T6: AStuntGuard (patrol/chase/stuck state machine, sight cone + line trace), ALaserBeam (activator-disabled), Caught cut reason | Build verified |
| 2026-11-12 | Claude Code | T7: AExplosiveProp (fuse, breaks BreakableWall-tagged actors, player cut, OnExploded, optional Niagara), APushCrate (kinematic sweep push, ghost push follows recorded position, kinematic fall) | Build verified |
| 2026-11-12 | Claude Code | T8: URetakeHUDWidget, URetakeResultsWidget, UCallSheetMenuWidget (widget trees built in C++), ARetakeHUD/MenuHUD, URetakeSaveGame, MainMenu map (Scripts/build_mainmenu.py), Space-to-roll | Build verified |
| 2026-11-12 | Claude Code | T9: Scene02_Heist and Scene03_Chase graybox levels via Scripts/build_scene02.py, build_scene03.py and shared Scripts/retake_levels.py (also Director's Cut cameras); PushCrate size is now a vector with corner-based support | Levels verified by script |
| 2026-11-12 | Claude Code | T10: UDirectorsCutComponent (camera cuts between tagged cine cameras, interest-based shot choice, grain/sepia/vignette post process), UDirectorsCutOverlayWidget (letterbox + title cards), Space to skip; LFS lockable removed | Build verified (1 compile fix) |
| 2026-11-12 | Claude Code | T10b: automated playtests (ARetakeTestDriver + Retake.AutoTest console command, Scripts/run_playtests.ps1) for all 3 scenes, solution + wrong solution; all pass, no gameplay bugs found (only test-script timing fixes) | Headless -game -nullrhi -benchmark |
| 2026-11-12 | Claude Code | Scene04_Finale (stretch): graybox via Scripts/build_scene04.py combining all set pieces, added to call sheet + cook list, playtests for solution + wrong solution; all 8 playtests pass; packaged | One layout fix (crate on lure path) |
| 2026-11-12 | Claude Code | Audio prep: URetakeAudioSubsystem (world subsystem listening to director/set-piece delegates), URetakeAudioSet data asset + URetakeSettings, sound classes/mix, volume sliders in call sheet (saved), Scripts/import_audio.py (slot-name import, verified with a test tone) | Playtests still pass |
