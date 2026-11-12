# RETAKE

*Epic AI MegaJam 2026 entry · theme: **"Director's Cut"** · solo entry by Markus, built with Claude Code · Unreal Engine 5.9 Preview, C++*

You are a stunt double on a chaotic film set. Every failed take ends with **"CUT! Take N"** – and stays in the shot.
Each earlier take replays as a sepia "ghost actor" at the same time as your new attempt, so your past takes have to
work together: one holds a pressure plate, one lures the security guard, one gets blown up by the explosive (it's a
stunt, after all), and the last one walks out a hero. When the scene is wrapped, all takes replay together as a
cinematic **Director's Cut** with camera cuts, letterbox and film grain.

## How to play
1. Read the **shot list** in the briefing, press **Space** to roll.
2. Do one part of the job, then press **R** to call *Cut*. That take now becomes a ghost.
3. Next take: the ghost repeats exactly what you did, in sync with you. Do the next part.
4. Reach the **mark** (blue disc) to wrap the scene. Fewer takes = more stars (par is on the dailies report).
5. Run out of takes and the director sends you back to Take 1.

Scenes: **The Bridge** (2 takes) · **The Heist** (3) · **The Chase** (3) · **The Finale** (4). Each unlocks the next on the call sheet.

## Controls
| Action | Keyboard / mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | Left stick / right stick |
| Jump · Roll (briefing) · Skip Director's Cut | Space | A |
| Interact (lever, explosive) | E | X |
| Cut (end the take) | R | Y |
| Pause | Esc (P in the editor) | Start |
| Push a crate | walk into it | walk into it |

## Set pieces
Pressure plates and gates · levers and security lasers · a patrolling guard (catches you, but gets stuck on ghosts) ·
explosives that break walls (and stunt doubles) · kinematic crates that can bridge a gap.

## Running it
- Packaged build: `Builds/Windows/EpicAiJam.exe` (starts in the main menu).
- From source: install Git + Git LFS (`git lfs install`), clone, then Unreal Engine 5.9 Preview + Visual Studio 2022
  (17.14+, C++ game workload). Right-click `EpicAiJam.uproject` → *Generate Visual Studio project files*, open it,
  let it build the module, press Play (the editor opens Scene 1).
- Automated playtests (all scenes, intended solution + a wrong one): `powershell -File Scripts\run_playtests.ps1`.

## Project layout
- `Source/EpicAiJam/Retake/` – all gameplay in C++ (recorder, ghosts, scene director, set pieces, UI, audio, tests).
- `Scripts/` – Python editor scripts that build the graybox levels and import art/audio; playtest runner.
- `Content/Retake/` – maps, materials, audio data. Blueprints are only used for placement/tuning/asset references.
- `ArtSource/`, `AudioSource/` – raw assets + naming rules for the import scripts.

## Submission docs
- [ASSETS.md](ASSETS.md) – external assets and licenses
- [PREEXISTING.md](PREEXISTING.md) – what existed before the jam
- [AI_LOG.md](AI_LOG.md) – how AI tools were used, step by step
- [DEVLOG.md](DEVLOG.md) – development log ("glass box")
