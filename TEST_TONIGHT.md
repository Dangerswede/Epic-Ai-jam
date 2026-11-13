# Test tonight

Numbered steps to try in the editor. Updated after each task.

> **Automated playtests: all 8 pass** (solution + wrong solution for Scene01/02/03/04). They drive the real player with simulated input at a fixed 60 fps and check ghosts, plates, gates, lasers, explosive, crate, Wrap -> Director's Cut -> Results and save/unlock. Re-run: `powershell -File Scripts\run_playtests.ps1` (logs in `Saved/Playtests`; uses its own save slot). Not covered by tests: how it *looks* and *feels* (camera, readability, timing at real frame rates), the main menu UI, the Space-skip in the Director's Cut - that's what tonight is for.
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

## Scene04_Finale (stretch, par 4, max 6, 45 s per take)
59. Open `Content/Retake/Maps/Scene04_Finale` (or menu → unlocked after Scene 3). One long run: sound stage → corridor → courtyard → back alley → rooftop.
    - Take 1: stand on the plate in the start room (opens the corridor gate).
    - Take 2: arm the barrel next to the wall in the corridor and get blown up.
    - Take 3: wait for the bang, go through into the courtyard, let the guard spot you (front of its patrol, around the middle of the courtyard), run to the far south-east corner.
    - Take 4: wait for the bang, enter the courtyard, wait ~2 s for ghost 3 to drag the guard away, pull the lever (north side), go through the laser door (east), push the long crate into the pit, run up the ramp to the rooftop goal.
60. Check: whole scene doable in under ~2.5 min; the rooftop (water tower, billboard) reads as a finale; Director's Cut shows 4 ghosts.

## Audio (prep for tonight's sounds)
61. Main menu: a **SOUND DEPARTMENT** row with MUSIC / SFX / VOICE sliders. Move them, restart the game: values are kept.
62. Drop WAVs into `AudioSource\` named like the slots (see `AudioSource\README.txt`, e.g. `VoiceAction.wav`, `SfxExplosion.wav`, `MusicRollingLoop.wav`) and run:
    `"E:\Unreal engine\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" D:\Projekt\EpicAiJam\EpicAiJam.uproject -run=pythonscript -script=D:\Projekt\EpicAiJam\Scripts\import_audio.py`
    (editor closed). Sounds land in `Content/Retake/Audio/Sounds` and are assigned in `DA_RetakeAudio`.
63. Play a scene: clapperboard + "Take two!" + "Action!" at each take, "Cut!" on R, "Get me a new stunt double!" when blown up, "That's a wrap!" at the goal, "Print it!" on results; music changes per state. Missing sounds are just silent.

## Art (prep for tonight's models)
64. Drop GLB/FBX + textures into `ArtSource\` (naming in `ArtSource\README.txt`, e.g. `SM_StudioSpotlight.glb`, `T_StudioSpotlight_BaseColor.png`) and run (editor closed):
    `"E:\Unreal engine\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" D:\Projekt\EpicAiJam\EpicAiJam.uproject -ExecutePythonScript="D:\Projekt\EpicAiJam\Scripts\import_art.py" -unattended -nullrhi`
65. Check in `Content/Retake/Art`: SM_ mesh with MI_ material (parent `M_RetakeMaster`) showing its textures, sensible size (1 unit = 1 cm). Fill in the new rows at the bottom of ASSETS.md.
66. Swap a placeholder: select e.g. the Gate in a level → Details → DoorMesh → pick the new SM_. (Tested with a generated cube + texture: import, MI creation, texture hookup and meters→cm rescale all worked.)

## Pause menu
67. In a scene press **P** (in the editor; **Esc** stops PIE there) or **Esc** in the packaged game: dimmed screen, "PAUSED – The crew is on a coffee break.", buttons RESUME / RESTART SCENE / MAIN MENU, cursor visible, game frozen (ghosts and timer stop).
68. RESUME (or P/Esc again) continues exactly where you were. RESTART SCENE starts again from Take 1. MAIN MENU goes to the call sheet.

## Docs (drafts, please review)
69. Read `README.md` (English: pitch, how to play, controls), `PREEXISTING.md` (fill in the TODOs: when the plan/prompts were written, other tools) and `DEVLOG.md` (glass-box devlog draft from AI_LOG + git history; add screenshots later).

## Code-review fixes (12 Nov, afternoon)
70. **P1 – actions only while Rolling:** In the briefing press Space: the take starts but the character does NOT jump. During the red "CUT!" pause press E next to a lever: nothing happens. During the Director's Cut, E/R do nothing (Space still skips).
71. **P2 – key hints:** while a take is rolling, a dark bar bottom-left reads "WASD Move   SPACE Jump   E Use   R Cut   Q Scrap take   ESC Pause" (hidden in briefing/cut/Director's Cut). Levers show a yellow "[E]" above them. Scene 1's shot list now ends with "Press R to call CUT when you're in position. Your take stays where you left it." Ideal check: let someone who hasn't played finish Scene 1 without telling them the keys.
72. **P3 – scrap take:** in take 3 press **Q** (Backspace only works in the packaged game; in the editor it's swallowed by the editor's Delete command): red stamp "SCRAPPED! Take 2 hits the cutting room floor", after ~1 s take 2 rolls again with only the take-1 ghost. Backspace in take 1 does nothing.
73. **Out of film:** in Scene 1 press R five times quickly: on take 5 the stamp says "OUT OF FILM! Last take scrapped" and take 5 rolls again (takes 1-4 stay). No more wipe to Take 1 (use pause → RESTART SCENE for that).
74. Results screen now shows "Takes in the cut: X (par Y)" and "Film shot: Z takes"; stars count takes in the cut.
75. **P4 – guard vs. finished ghosts:** a ghost whose take has ended no longer attracts the guard (and a guard stuck on a ghost lets go when that take ends). So a lure only works while the luring take is still playing: in **Heist** take 1, get spotted, then KEEP RUNNING (down to the south-west corner) until the guard catches you; in take 2 wait until the guard has followed ghost 1 south before going to the lever. Take time in Heist is now 30 s. In **Finale** take 3, after being spotted run to the south-east corner and then west along the south wall. Check it feels fair (test margin: ~5 s in Heist, more in Finale).

## Stunt feel + Director's Cut edit (12 Nov, P5-P7)
76. **P5 – stunt death:** in Scene 3 arm the barrel and stay close: brief slow-mo, camera shakes, you're thrown ~6 m through the air, then "CUT!". Same for lasers and getting caught by a guard (thrown away from the guard). Next take your ghost flies the same arc and then **stays lying down** (it doesn't press plates and the guard ignores it). Falling into a pit still cuts instantly.
77. **P6 – faithful Director's Cut:** in Scene 3 and 4 the Director's Cut now plays back what the guard and the crate actually did in the winning take (no re-simulation). Check: the crate drops into the pit at the same moment and spot as when you played, and no ghost walks on air over the pit; the guard's sign (SECURITY / HEY! STOP! / GOTCHA!) matches. Optional: console `t.MaxFPS 20` before wrapping and check the same.
78. **P7 – Director's Cut as an edit:** ghosts become solid-coloured "actors" (winning take red, others muted blue/olive/teal...), no TAKE labels; a small "TAKE N" (or "ALL TAKES") label bottom-left at each cut. Pacing: slow-mo (0.35x) around explosions and the hero's last second, fast-forward (3x) when nobody moves, a brisk 2.2x "cruise" when people walk but nothing is about to happen, 1x around actions. Always cuts to the camera nearest an explosion, ends on the hero. Colours more saturated. Test lengths: Scene 1 ~8 s, 2 ~14 s, 3 ~16 s, 4 ~23 s (target < 25 s). Judge: does it feel like a highlight reel, is slow-mo/fast-forward too jumpy?

## Evening tasks P8-P10 (12 Nov)
79. **P8 – new Scene 4 opening:** two marks (north and south plates) – the gate only opens when BOTH are pressed. A small crate sits between them: push it onto the north mark and stand on the south mark yourself (crates press plates and now slide over the plate edge). No bomb/wall in Scene 4 any more. Par 4; the crate trick does it in 3 takes.
80. **P9 – director's notes:** the briefing card shows "DIRECTOR'S NOTE: ...". Stars: 1 = wrapped, 2 = at/under par, 3 = at/under par AND the note. Notes: Scene 1 final take under 12 s, Scene 2 nobody gets busted, Scene 3 final take under 20 s, Scene 4 no scrapped takes. Dailies report shows [X]/[  ] for the note; the call sheet shows best stars per scene.
81. **P10 – timeline:** while rolling, a strip at the top centre shows one thin row per earlier take: gold ticks where that take pressed E, a red X where a take ended in a stunt death, a light stop mark where it ended otherwise, and a red playhead moving with the current take. Check that it helps you time things.
82. **P11 – stand on a frozen ghost (Scene 5: The Gap, unlocked after Scene 4):** a 192 cm deep, 8 m long stunt pit you can't jump. Take 1: drop into the pit and stand near the far end, CUT. Take 2: drop in and stand near the near end, CUT. Take 3: hop across their heads (they're flush with the floor once their take is over; a ghost that died lies down and can't be used). Check: does landing on a head feel fair (platform is 80 x 80 cm)?

## Art pass (12 Nov evening)
83. Every scene now has Tripo props as set dressing (camera on tripod, director's chair + clapperboard, studio spotlights, scaffolding towers, plywood flats, stage crates), placed off the play paths and without collision. Pressure plates use the metal plate prop, levers the wall lever (the coloured graybox handle still shows on/off), the Scene 3 explosive is the dynamite bundle, the small Scene 4 crate and the cover crates are stage crates. The long bridge crates are still graybox. Check: scale and placement of props, nothing blocks a path, nothing looks floating. Note: the first time you open a level the editor compiles the 2K prop textures; they may look grey for a minute.

## C1 – Stunt double character
84. The player is now the UE mannequin in a white-and-red stunt suit with a retro motorcycle helmet (Tripo prop on the "head" socket). Check: walking/running/jumping animate, the helmet sits right (tweak in Project Settings → Game → Retake → Character: Helmet Size/Offset/Rotation), the stunt launch looks OK. Ghosts use the mannequin with the translucent sepia ghost material; in the Director's Cut they are solid per-take colours; a ghost that died lies on its side. Scene 2's money bag is now the Tripo money bag; film reel cans were added to the set dressing.

## D1 – Roles and timing windows (mechanics; scenes follow in D2-D5)
85. New gameplay piece **TimedSwitch** (post with a bar on top and a label): press E (or a ghost does) and it stays on for 5 s, the bar shrinks, the label counts down; press again to restart. A gate linked to several timed switches with "require all" is only open while all of them are on at once. Not placed in any scene yet (D2+).
86. Scenes can now give each take a **role** ("TAKE 2 - INSIDE MAN: Hit all three locks inside 6 seconds."): shown on the briefing card, again on the card during the CUT pause (for the next take), in gold under the clapperboard while rolling, and on the ghosts' labels. No scene has roles yet (D2+).

## R1 – Optional highlight reel
87. After the wrap you now go straight to the **Dailies report**. It has a **WATCH DIRECTOR'S CUT [SPACE]** button; the reel then plays and returns to the report.
88. The reel is a highlight cut (tests: Scene 1 ~5 s, 2 ~7 s, 3 ~8 s, 4 ~11 s): only moments where something happens (E presses, stunt deaths, the hero's arrival). Dead time is skipped behind a black frame. Slow-mo around stunt deaths, explosions and the hero's last second.
89. Camera: one moving camera that follows the actor of each moment: chase cam behind them, low side angle for stunt deaths, a final wide shot on the hero. If a wall is in the way it picks another angle (side, then overhead). The placed DC_Cam actors are no longer used. Small "TAKE N - ROLE" label at each moment. Space skips. Judge: is it worth watching now?

## D2 – Scene 1 redesign (tutorial)
90. **Scene 1: The Bridge** now teaches timed switches. Two gates on the bridge, each opened for 5 s by its own switch on the studio floor. TAKE 1 - GRIP: hit SWITCH A by the ramp, sprint under the bridge, hit SWITCH B, R. TAKE 2 - HERO: run up the ramp and through both gates while they're open. 15 s per take, par 2, director's note: final take under 9 s. Whole scene ~1 min. Check: is it obvious what to do from the shot list + role card alone?

## D3 – Scene 2 redesign
91. **Scene 2: The Heist** teaches roles (12 s per take, par 3, ~45 s total). The vault door is a gate that only opens while LOCK A, B and C (north wall) are all on (6 s each). TAKE 1 - DECOY: get spotted by the guard (it walks toward you), run to the south-west corner and then east along the south wall until the take ends. TAKE 2 - INSIDE MAN: go up the west side, along the north wall, hit the three locks quickly, R. TAKE 3 - HERO: wait a moment until the guard turns after the decoy, go south-east and into the vault while the locks are open, grab the bag. Lasers and the lever are gone from this scene. Director's note: nobody gets busted.

## D4 – Scene 3 redesign
92. **Scene 3: The Chase** - new trick EXPLOSIVES with the old timed switch (16 s takes, par 3). TAKE 1 - PYRO: light charge 1 (2 s fuse), back off, after the bang light charge 2 (0.5 s fuse) - you get blown up (stunt). TAKE 2 - GRIP: take the north corridor from the start room and hit the EXIT switch at its end. TAKE 3 - HERO: wait out the first bang near the start of the corridor, wait out the second one before the second wall, then run for the exit while it's open. The crate and the pit are gone from this scene. Director's note: final take under 18 s.

## D5 – Finale order planning
93. **Scene 4: The Finale** - the clapper shows CAST: GRIP / DECOY / INSIDE MAN / HERO and you choose the order (30 s takes, par 4). A working order: TAKE 1 - GRIP: push the small crate onto the north plate, stand on the south plate (the gate opens), R. TAKE 2 - DECOY: through the gate, let the guard see you, then run in a big circle in the south part of the courtyard until the take ends. TAKE 3 - INSIDE MAN: wait by the west wall until the guard is south, pull the lever (north wall), R. TAKE 4 - HERO: same wait, the north side of the courtyard, through the lasers, push the crate into the pit, up the ramp to the roof. Check: does a wrong order (for example INSIDE MAN before DECOY) feel fair and understandable?

## F1 – Camera marks and Director's rating
94. Every scene has 3-4 **camera marks**: a white tape ring on the floor with a camera on a stand and a label like MARK 2  3-6s. During a take the ring pulses green with NOW! while its window is open; stand on it and you get the stamp ON MARK! and the ring turns gold (IN THE CAN). Check: are the marks easy to see and understand, and are any in a bad spot or blocking the way?
95. **Results screen:** the DIRECTOR'S RATING meter fills up with popups (WRAP +45, ON PAR +20, NOTE +20, ON MARK x2 +20), the stars light at 45 / 65 / 85 and it ends on PRINT IT! (3 stars) or ... Check that it doesn't feel slow (about 2 s).
96. **Director's Cut:** it should now cut between the mark cameras (bottom label: MARK 1: UNDER THE BRIDGE and so on). Check the camera angles - is any of them ugly or blocked by a wall?

## F3 – Explosions with punch
97. **Scene 3**: light charge 1 and watch: an orange fireball, smoke and sparks, a white flash on the screen, the camera shakes, everything goes slow-mo for a split second and wall chunks fly. Check: is the white flash too strong or annoying, and do the chunks ever get in the way (they shouldn't block you)?

## F4 – 70s popping text
98. Stamps (ACTION!, CUT!, ON MARK!, THAT'S A WRAP!), the rating popups and PRINT IT! now pop in big with an overshoot, a little wobble and a lean, plus a thick brown outline and a drop shadow. Check that they're readable and don't cover the action for too long.

## ST1 – Story plumbing (The Last Take)
99. **Menu:** says Director: still waiting; scenes are listed under REEL 1 / REEL 4 (the Gap is now Scene 16). After wrapping the Gap the menu also shows Production wrapped. Nov 14, 1977.
100. **Scene start:** the card shows CALL SHEET - NOV 14, 1977 - SCENE N: TITLE and a notes line; QUIET ON SET! pops, then the director's line (e.g. scene 3: Somebody check that gap rig...). The Gap shows SCENE 99: THE GAP and the director stays silent.
101. **Take numbers** start at 46 (clapper TAKE 46  FILM 1/5, CUT! Take 46, ghost labels).
102. **After scene 4, NEXT SCENE** shows a black leader card with a crosshair, 3-2-1 and END OF REEL 1, then loads the next scene.

## F6 – 70s film look
103. The whole game is a little warmer and grainier with a soft vignette. Check that it isn't too dark or muddy on your screen.
104. In the Director's Cut you also get flickering and thin white scratches that jump around (only in the replay). Too much or just right?

## ST2 – New scenes
105. **Scene 8: The Bridge (Reshoot)** (after scene 4 and the END OF REEL 1 card): dusk light; one gate needs SWITCH A and SWITCH B on at the same time (4 s each). Take 46 GRIP: wait by switch A until ~5 s, then press it. Take 47 BEST BOY: run under the bridge to switch B and press it. Take 48 HERO: wait a couple of seconds, then up the ramp - the gate opens in front of you.
