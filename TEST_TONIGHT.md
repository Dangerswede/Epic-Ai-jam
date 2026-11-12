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
