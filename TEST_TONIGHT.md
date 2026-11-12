# Test tonight

Numbered steps to try in the editor. Updated after each task.

> **Automated playtests (T10b): all 6 pass** (solution + wrong solution for Scene01/02/03). They drive the real player with simulated input at a fixed 60 fps and check ghosts, plates, gates, lasers, explosive, crate, Wrap -> Director's Cut -> Results and save/unlock. Re-run: `powershell -File Scripts\run_playtests.ps1` (logs in `Saved/Playtests`; uses its own save slot). Not covered by tests: how it *looks* and *feels* (camera, readability, timing at real frame rates), the main menu UI, the Space-skip in the Director's Cut - that's what tonight is for.
Open `EpicAiJam.uproject` (if asked to rebuild modules, say Yes).

> Note: `lockable` was removed from `.uasset`/`.umap` in `.gitattributes` so maps/assets are no longer read-only after a commit. If the editor still says a file is read-only, right-click it in Windows → Properties → untick *Read-only*.

## T0 – Character
1. Open any level (e.g. a new *Basic* level), press **Play** (PIE).
2. You spawn as a grey cylinder. WASD moves, mouse looks, Space jumps.
   Expected: camera follows behind on a spring arm, the body turns toward the movement direction.

## T1 – Take recorder
3. In PIE, walk around for ~5 s, jump a couple of times, press **E** a few times.
4. Press **R** ("Cut"). The Output Log shows: `LogRetake: CUT! Take 1: N frames, X s`.
   Expected: N ≈ X × 30 (e.g. 5 s → ~150 frames).
5. Open the console with **`** (backtick / § key on Swedish keyboard) and type `Retake.DumpTake`.
   Expected: `[RetakeCharacter_0] Take 1: N frames, X s (jump J, interact I, push 0)`; J and I match your presses.
6. Press R again, then `Retake.DumpTake` again. Expected: now says Take 2.

## T2 – Ghosts
7. In PIE, walk a short route, press **E** once, jump once, then press **R**.
8. Console: `Retake.SpawnGhost`. Expected: a cylinder with floating "TAKE 1" (facing the camera) replays your route exactly and stops/freezes at the last position.
9. Ghost tint: no ghost material exists yet, so the ghost looks like the player (grey). Known/expected until a translucent material `M_Ghost` (params `Tint`, `Opacity`) is created and assigned to `GhostMaterial`.

## T3 – Scene director (take loop)
10. In any test level: Place Actors panel → search **RetakeSceneDirector** → drag it in. (Optional: set *Take Duration Seconds* to 8 for faster testing.)
11. Play. Expected Output Log: `State: None -> Briefing`, after 4 s `-> Rolling`, you can move.
12. Walk somewhere, press **R**. Expected: `CUT! Take 1 (PlayerCut)`, control frozen ~2 s, then you are back at the start and a ghost "TAKE 1" replays your route while you play take 2.
13. Do R a few more times: ghosts for every earlier take play at once. After take 5: `SceneFailed`, then restart at take 1 with no ghosts.
14. Stand still and let the timer run out: cut reason should be `TimeUp`.

## T4 – Set pieces (test in any level with a RetakeSceneDirector)
15. Place **PressurePlate**, **Gate**, **Lever**, **GoalZone**, **KillVolume** from Place Actors (search by name).
16. Select the Gate → Details → *Linked Activators* → add the PressurePlate and the Lever.
17. Play. Stand on the plate: plate turns green and sinks, gate slides up. Step off: gate closes.
18. Walk to the lever, press **E**: handle flips and turns green, gate opens. Press E again: closes.
19. Press R, then in take 2: the ghost should press the plate/flip the lever exactly when you did, and the gate reacts to the ghost.
20. A ghost that ended standing on the plate keeps it pressed (gate stays open).
21. Walk into the GoalZone (blue disc): log shows `That's a wrap!` → `DirectorsCut` (all takes replay together, you're hidden) → `Results`.
22. Walk into the KillVolume (red wireframe box, visible in editor): `CUT! ... (Killed)`. A ghost walking through it does nothing.

## T5 – Scene01_Bridge (graybox)
23. The editor now opens **Scene01_Bridge** by default (Content/Retake/Maps). Press Play.
24. Briefing 4 s, then Take 1. Walk to the **red plate** on the floor (right side, near the far end of the bridge) and stand on it. The orange gate on the bridge should open. Press **R** (or wait for the timer).
25. Take 2: a translucent sepia ghost "TAKE 1" walks to the plate and stays there. Go up the **ramp**, across the bridge through the open gate, into the **blue disc** at the far end.
    Expected: `That's a wrap!` → Director's Cut (both takes replay together) → Results.
26. Check it can't be done in 1 take: stand on the plate, then run for the gate. It should close long before you get there.
27. Look check: lighting (sun + 2 spotlights), ghost is see-through and sepia. Note anything too dark/bright.
28. Packaged build: run `Builds\Windows\EpicAiJam.exe`. It should start straight into Scene01_Bridge. Alt+F4 to quit (no menu yet).

## T6 – Guard and laser (test in Scene01 or any level with a director)
29. Place a **StuntGuard**. In the viewport drag its diamond (*Patrol Point B*) to set the patrol route. Play: it walks back and forth, label "SECURITY".
30. Walk in front of it (within ~8 m, inside its view, yellow visor = front): label "HEY! STOP!", it chases you (slower than you). If it reaches you: `CUT! ... (Caught)`.
31. Take 2: let your take-1 ghost walk past the guard. Guard chases the ghost, says "GOTCHA!", stays glued to it ~3 s while the ghost keeps walking, then returns to patrolling.
32. Place a **LaserBeam** (red line, length = *Beam Length*). Walk into it: `CUT! (Killed)`. Ghosts walk through it.
33. Add a Lever/PressurePlate to the laser's *Linked Activators*: when active the beam disappears and is safe.

## T7 – Explosive and crate
34. Place an **ExplosiveProp** (red barrel, "[E] ARM") and a cube StaticMeshActor next to it. On the cube: Details → *Actor → Tags* → add `BreakableWall`.
35. Press **E** at the barrel: countdown 3-2-1 with blinking, then a yellow flash. The wall within 4 m disappears. If you stood within 4 m: `CUT! (Killed)`.
36. Next take: the wall is back. Your ghost arms the barrel at the same time as you did; the explosion breaks the wall again but doesn't hurt you unless you're near.
37. Place a **PushCrate** (brown cube). Walk into one side and keep holding the key: it slides away from you slowly. It stops at walls. It doesn't rotate or tumble.
38. Push it over a ledge/gap: it drops straight down. In the next take the ghost pushes the crate along exactly the same path.
39. Push the crate onto a PressurePlate: the plate activates.

## T8 – HUD, menu, results
40. Scene01 (editor default): HUD shows a clapperboard top-left (RETAKE PRODUCTIONS / scene title / TAKE 1 / 5) and the shot list at the bottom with "Press Space to roll". Nothing happens until you press **Space**.
41. Space: big yellow "ACTION!" stamp slams in, countdown top-right (red under 5 s).
42. Press R: red "CUT! Take 1" stamp. Kill volume/guard/time-up add "Ouch." / "Busted." / "Too slow!".
43. Use all 5 takes: "SCENE FAILED / Back to Take 1".
44. Wrap the scene: "THAT'S A WRAP!", Director's Cut, then a **Dailies report** (paper sheet): takes used, par, 3 diamonds (gold = stars; 3 if takes <= par 2), buttons NEXT SCENE / RETRY / MAIN MENU (mouse cursor visible).
45. Open `Content/Retake/Maps/MainMenu` and Play (or run the packaged game; it now starts here): **Call sheet** with 3 scene rows. Scene 1 READY, 2 and 3 LOCKED (greyed). After wrapping Scene 1 it shows WRAPPED (N takes) and Scene 2 unlocks (Scene 2/3 maps arrive in T9).
46. QUIT closes the game (in PIE it ends the session).
47. Reset progress: delete `Saved/SaveGames/RetakeProgress.sav`.

## T9 – Scene02_Heist and Scene03_Chase (open from Content/Retake/Maps, or via the menu once unlocked)
48. **Heist** (par 3): vault at the far end behind 3 red lasers; lever in the right-hand alcove area; guard patrolling in front of it.
    - Take 1: walk into the guard's view, then run far away (e.g. back-left corner). Getting caught is fine.
    - Take 2: while the guard chases ghost 1, pull the lever (E). Lasers disappear.
    - Take 3: walk through the door, into the vault, onto the money bag (blue disc).
    Check: guard really follows ghost 1 long enough; lasers kill you when on; 3 takes feels right (2 might be possible = fine).
49. **Chase** (par 3): plate in the start room opens the corridor gate; red barrel next to a wall; a long brown crate; a wide pit; exit at the end.
    - Take 1: stand on the plate.
    - Take 2: go to the barrel, press E. Fuse is only 0.5 s and radius 5 m: you get blown up (that's the stunt).
    - Take 3: stay in the start room until the bang, then go through, push the long crate (walk into its back) until it drops into the pit, walk over it, reach the exit.
    Check: crate drops flush with the floor; the pit can't be jumped; falling in = CUT.
50. Unlock flow: wrap Scene 1 → menu shows Scene 2 READY, etc.
51. Packaged build: `Builds\Windows\EpicAiJam.exe` starts in the main menu.

## T10 – Director's Cut
52. Wrap any scene. After "THAT'S A WRAP!" the HUD disappears, black letterbox bars appear and the title card "DIRECTOR'S CUT" + scene title fades in over a wide shot.
53. All takes (including the winning one) replay together from the start. The picture is grainy, desaturated/sepia with a vignette.
54. Every 2.5-4 s it hard-cuts to another placed camera, preferring the one nearest a ghost that just did something (pressed E, jumped, pushed); the camera slowly follows that ghost.
55. At the end: wide shot + "THAT'S A WRAP / N takes", then the Dailies report.
56. Press **Space** during the Director's Cut: it skips straight to the Dailies report.
57. Cameras are the `DC_Cam1..4` actors (folder *Cameras*) in each map; move them freely. A level with no tagged cameras gets one automatic wide camera.
58. Packaged build: menu → Scene 1 → full loop incl. Director's Cut works outside the editor.
