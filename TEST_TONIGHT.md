# Test tonight

Numbered steps to try in the editor. Updated after each task.
Open `EpicAiJam.uproject` (if asked to rebuild modules, say Yes).

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
