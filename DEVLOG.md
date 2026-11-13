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

### Day 1, afternoon – playtest fixes (P1–P11)
The developer's first hands-on session produced a list; each item became one small commit with a playtest.
- **P1–P4**: block actions outside a rolling take, key hints, *scrap last take* (Q) instead of "out of film wipes
  everything", guards ignore ghosts whose take is over.
- **P5 stunt deaths**: launch, 0.3x hit-stop, the flight is *recorded* so the ghost replays the fall and stays down.
- **P6–P7**: the Director's Cut replays the winning take's *world track* (guards, crates) instead of re-simulating –
  replay error measured at < 1 cm by the test.
- **P8–P11**: a crate can hold a plate, director's notes + 3-star rating, a HUD timeline of earlier takes, and
  "stand on a frozen ghost" (head platforms) with a new scene, *The Gap*.

### Day 1, evening – art and the "fun pass"
- **Tripo3D props** (GLB) imported by `import_art.py`; gameplay actors keep their graybox as invisible collision and
  show the prop. Mannequin character with a stunt suit and a Tripo helmet.
- An interview with the developer produced a ranked **fun list (F1–F8)** and a redesign of every scene around
  **roles** (GRIP, DECOY, INSIDE MAN, HERO...): each scene introduces exactly one new thing, the finale makes you
  plan the *order* of takes. A playtest metric flags any take that stands still for more than 3 s.
- **F1 camera marks + Director's rating** (a meter that gives the stars), **F3 explosions** (Niagara template +
  a hand-made glow fireball, flash, shake, debris), **F4 70s popping text**.

## Day 2 (13 Nov) – "The Last Take": 16 scenes in one night
The developer approved a story: you play Jack Ward, a stunt double on a 1977 picture, repeating his last shooting
day – he died doing the gap jump. The reveal is told only through text, set dressing and the director's voice.
No new mechanics: new scenes recombine the existing actors.

| Time | Commit | What happened |
|---|---|---|
| 02:31 | ST1 | Story plumbing: a 16-scene table (4 reels), call sheets, the director's line per scene, "END OF REEL" leader cards, take numbers start at 46. |
| 02:35 | F6 | 70s film look (grade, grain, vignette; scratches + flicker only in the replay). |
| 02:39–02:48 | ST2 | Must-have scenes 8, 10, 12, 15 – each with a solution playtest and a "wrong way" playtest. |
| 02:55 | S5 | The ending: music drops, two quiet lines, a typewriter memorial card, "DIRECTOR'S CUT – FOR JACK". |
| 03:10 | ST3 | Story set dressing (crossed-out crew board, a wreath "FOR JACK"); Tripo props swap in automatically. |
| 03:13–03:53 | +7 scenes | Rooftop, Vault, Projection Room (a live "dailies" screen), Heist (Night), Chase (Final), Saloon, Backlot (+ a free-roam *location scout* before take 1). All 16 scenes built; 32/32 playtests. |
| 06:59–07:35 | Review | A code/finish review by the developer: the Director's Cut plays by itself at the big moments, roles become hints ("H: ask the director"), Space rolls each take, T retakes, SKIP SCENE after struggling, ghosts no longer shove you, audio file list, THE END + credits. |

## What worked
- **Determinism by design.** Recording positions + actions (not inputs) at a fixed rate made ghosts reliable
  from the first build; the kinematic crate follows the ghost's recorded position instead of simulating.
- **Levels as code.** Python editor scripts rebuild a level in seconds and are diff-able; the AI could
  verify every level by dumping its actors.
- **Testing without a human.** Headless playtests at a fixed 60 fps let the assistant prove each scene is
  solvable (and not solvable the wrong way) while the developer was away.

## What went wrong / lessons
- **Pursuit maths beat trial and error.** Decoys kept getting caught on small circles; a guard at 380 cm/s chasing
  a runner at 500 cm/s on a circle of radius R gets within R x 0.24 of them, so R must be > 4.6 m to stay out of
  the 110 cm catch distance. Fixed once, on paper.
- **Look at the picture.** Several visual bugs (an explosion fireball hidden behind its own flash sphere, a
  projection screen facing the wall, unreadable signs) were only found by taking 1080p screenshots from scripted
  cameras (`-RetakeBlastShot`, `-RetakeLookAt`) and actually looking at them.
- **Two attempts, then simplify** kept the night moving: e.g. the live projection screen fell back to a plain lit
  screen until the real cause (orientation) was found.
- A Python commandlet silently failed to save a re-opened map → traced to read-only LFS files.
- Some editor subsystems don't exist under `-run=pythonscript`; the art importer needs `-ExecutePythonScript`.
- Test scripts initially walked too close to the guard and got "caught" – the game was right, the test wasn't.

## Still to do (TODO)
- Audio: 41 files listed in `AudioSource/README.txt` (director voice incl. story lines, 13 SFX, 2 music loops).
- The display font (Bowlby One, OFL) - the popping-text code is ready for it.
- Evening playtests by a human: feel, readability, camera, difficulty.
- Screenshots / GIFs for this log and the submission page.
