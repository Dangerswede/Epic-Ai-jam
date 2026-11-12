# Retake — Epic AI MegaJam 2026

Unreal Engine 5.9 Preview C++ project (module `EpicAiJam`). Theme: **"Director's Cut"**. Game: **"Retake"**.

## Pitch
The player is a stunt double on a chaotic film set. Each failed take ends with **"CUT! Take N"**.
Every earlier take replays simultaneously as a ghost "actor" in the same scene, so takes must
cooperate (hold pressure plates, distract a guard, push crates). When the scene goal is reached,
all takes replay together as a cinematic **"Director's Cut"** with camera cuts.

## Rules
- All game logic in C++. Blueprints only for placement, tuning values and asset references.
- All in-game text and audio in English.
- No third-party IP.
- Gameplay content must be created during the jam (12–15 Nov 2026).
- Teen rating.

## Recording rule
Takes record **transform + discrete actions at a fixed 30 Hz**, never raw input or physics,
so playback can't desync.

## Planned classes
Module `EpicAiJam`, folder `Source/EpicAiJam/Retake/`:

| Class | Purpose |
|-------|---------|
| `URetakeRecorderComponent` | Samples owner transform + actions at 30 Hz |
| `FTakeData` / `FTakeFrame` | Recorded take data |
| `ATakeGhost` | Replays a previous take |
| `ARetakeSceneDirector` | Owns takes, resets scene, spawns ghosts |
| `IRetakeResettable` | Interface for actors that reset on a new take |
| `APressurePlate` | Activated while something stands on it |
| `AGate` | Opens from plates/levers |
| `ALever` | Interactable toggle |
| `AExplosiveProp` | Stunt prop |
| `APushCrate` | Pushable crate |
| `AGoalZone` | Scene goal |
| `AStuntGuard` | Guard that can be distracted |
| `UDirectorsCutComponent` | Final cinematic replay with camera cuts |
| `URetakeHUDWidget` | HUD ("CUT! Take N" etc.) |
| `ARetakeCharacter` | Player character (done in T0) |

## Workflow
- Build after every change:
  `"E:\Unreal engine\UE_5.9\Engine\Build\BatchFiles\Build.bat" EpicAiJamEditor Win64 Development -Project="D:\Projekt\EpicAiJam\EpicAiJam.uproject" -WaitMutex`
- Max 2 attempts per approach before trying a simpler one.
- Log each AI-assisted step in `AI_LOG.md`.
- List every external asset in `ASSETS.md`.

## Developer
The developer is new to Unreal and comes from Godot: when explaining, give the Godot equivalent in one line.

## Task plan
Full task plan with prompts T0–T15: see `docs/RETAKE_PROMPTS.md` (Markus may copy it in).
