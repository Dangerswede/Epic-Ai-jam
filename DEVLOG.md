# Retake – Devlog (glass box)

*Draft. How a solo developer, new to Unreal and coming from Godot, built "Retake" with an AI assistant
(Claude Code) in the Epic AI MegaJam 2026. Times are local (CET). Full step list: [AI_LOG.md](AI_LOG.md);
every step is a commit in the git history.*

## Setup and rules (before the jam, 11 Nov)
- Empty UE 5.9 Preview C++ project, Git LFS, doc templates. Toolchain: VS 2022 17.14 + Windows SDK.
- Ground rules written into `CLAUDE.md` for the AI: *all game logic in C++, Blueprints only for placement and
  tuning; build after every change; max two attempts per approach, then simplify; log every AI step;
  explain things with the Godot equivalent.*
- Key design rule for the core mechanic: **takes record transform + discrete actions at a fixed 30 Hz, never raw
  input or physics**, so ghost playback cannot desync.

## Day 1 (12 Nov) – from empty project to four playable scenes
Working mode: the developer approved a task, the assistant implemented, built, committed and reported in Swedish;
the developer tests in the evening, so everything was also written into a running `TEST_TONIGHT.md` checklist.

| Time | Commit | What happened |
|---|---|---|
| 06:35 | T0 | Character (spring-arm camera, Enhanced Input built in C++, no input assets needed) + game mode. |
| 06:38 | T1 | Take recorder: fixed 30 Hz sampling with accumulated delta time, action bit flags (Jump/Interact/Push). |
| 06:40 | T2 | Ghost replay with interpolation; ghosts use *the same interact code path* as the player. One compile fix. |
| 06:42 | T3 | Scene director state machine (Briefing → Rolling → Cut → … → Wrap → Director's Cut → Results). |
| 06:45 | T4 | Pressure plate, gate, lever, goal, kill volume – all resettable per take. |
| 06:52 | T5 | First level built by a **Python editor script** run headless (no MCP available on that machine) + first packaged build. |
| 06:56–06:58 | T6–T7 | Guard (simple C++ state machine, sight cone), laser, explosive, kinematic crate (no physics, so ghosts push it identically). |
| 07:03 | T8 | HUD, call-sheet main menu, results screen, save game – widget trees built in C++ (no UMG assets to keep in sync). |
| 07:11 | T9 | Heist and Chase levels by script; crates got non-cube sizes to bridge a gap too wide to jump. |
| 07:13 | – | Found that Git LFS `lockable` made every committed map read-only (editor and scripts couldn't save) → removed. |
| 07:14 | T10 | Director's Cut: cuts between placed cine cameras, preferring the one nearest the ghost that just acted; grain, sepia, letterbox, title cards. |
| 07:21 | T10b | **Automated playtests**: a test driver plays every scene headless with simulated input (intended solution + a wrong one). All passed; no gameplay bugs, only test-script timing fixes. |
| 07:27 | Finale | Scene 4 combining every set piece; first test run caught a decoration crate blocking the lure path → moved. |
| 07:33–07:38 | Prep | Audio subsystem + data asset (empty slots silent), art import pipeline + master material, pause menu. |

## What worked
- **Determinism by design.** Recording positions + actions (not inputs) at a fixed rate made ghosts reliable
  from the first build; the kinematic crate follows the ghost's recorded position instead of simulating.
- **Levels as code.** Python editor scripts rebuild a level in seconds and are diff-able; the AI could
  verify every level by dumping its actors.
- **Testing without a human.** Headless playtests at a fixed 60 fps let the assistant prove each scene is
  solvable (and not solvable the wrong way) while the developer was away.

## What went wrong / lessons
- A Python commandlet silently failed to save a re-opened map → traced to read-only LFS files.
- Some editor subsystems don't exist under `-run=pythonscript`; the art importer needs `-ExecutePythonScript`.
- Test scripts initially walked too close to the guard and got "caught" – the game was right, the test wasn't.

## Still to do (TODO)
- Art and audio made during the jam (pipelines are ready: `ArtSource/`, `AudioSource/`).
- Evening playtests by a human: feel, readability, camera, difficulty.
- Screenshots / GIFs for this log and the submission page.
