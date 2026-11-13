"""
Scene09_Backlot - reel 3 opener, and the first scene with F7: LOCATION SCOUT (walk the set before take 1,
nothing recorded, marks visible, Space rolls). Night on the studio backlot. Old pieces: a guard + an explosive.
A fake street facade blocks the way; one boarded-up section can be blown open. The guard watches the street.
  take 1 DECOY: let the guard see you (north side), lead it round a big circle in the north-west.
  take 2 PYRO: south route to the charge by the facade, light it, get clear (it's a long fuse).
  take 3 HERO: south route, wait out the blast, through the hole to the goal.
A sign on the facade: NO SHOOTING TODAY.

Layout (cm): lot x -900..3200, y -1000..1000. Facade x 1780..1820 across the lot; boarded section y -400..-100.
Charge at (1700, -300), fuse 3 s, radius 350. Guard at (1300, 600) facing west, walking to (700, 600).
Goal behind the facade at (2800, -250).
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene09_Backlot"
L.new_map(MAP)
H = 380

L.box_span("Lot", -900, 3200, -1000, 1000, -20, 0)
L.box_span("Fence_N", -900, 3200, 1000, 1030, 0, 300)
L.box_span("Fence_S", -900, 3200, -1030, -1000, 0, 300)
L.box_span("Fence_W", -930, -900, -1030, 1030, 0, 300)
L.box_span("Fence_E", 3200, 3230, -1030, 1030, 0, 300)
L.box_span("Facade_N", 1780, 1820, -100, 1000, 0, H)
L.box_span("Facade_S", 1780, 1820, -1000, -400, 0, H)
L.box_span("Facade_Top", 1780, 1820, -400, -100, 300, H)
boards = L.box_span("BoardedDoor", 1780, 1820, -400, -100, 0, 300)
L.tag(boards, "BreakableWall")
kill = L.spawn(unreal.KillVolume, (1100, 0, -1200), label="KillVolume_Net")
kill.set_actor_scale3d(unreal.Vector(20, 20, 1))

charge = L.spawn(unreal.ExplosiveProp, (1700, -300, 45), label="Charge")
charge.set_editor_property("fuse_seconds", 3.0)
charge.set_editor_property("radius", 350.0)
L.set_art(charge, "Dynamite")

guard = L.spawn(unreal.StuntGuard, (1300, 600, 100), rot=(0, 180, 0), label="Guard")
guard.set_editor_property("patrol_point_b", unreal.Vector(600, 0, 0))  # yaw 180: walks west first
guard.set_editor_property("lose_sight_multiplier", 3.0)

L.spawn(unreal.GoalZone, (2800, -250, 100), label="GoalZone_Backlot")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(sun_rot=(-25, 160, 0), spots=[((800, -900, 900), (1400, -300, 0)), ((2400, 700, 900), (2600, -200, 0))])
L.mood(sun_rot=(-25, 160, 0), sun_color=(0.4, 0.5, 0.85), sun_intensity=0.3, sky_intensity=0.12, spot_scale=0.9)

# S3: the sign, and the clock (4:47 from scene 7 on).
L.sign("NO SHOOTING TODAY", (1765, 450, 230), yaw=180, size=40, color=(0.95, 0.85, 0.3), label="Sign_NoShooting")
L.wall_clock((1765, 800, 260), 180)
L.dressing("StudioLight", (-700, 800, 0), (60, 60, 260), color=(0.12, 0.12, 0.12))

L.cameras([((-800, -900, 800), (1000, 0, 0), 18), ((1300, -950, 500), (1800, -250, 100), 28)])
d = L.director(start, "Scene 9: The Backlot",
               "Night, and the lot is closed. LOCATION SCOUT: walk the set first - nothing is recorded until you press Space. "
               "A guard watches the street and the boarded door in the facade won't open by itself.",
               par=3, max_takes=5, duration=14, bonus=("NOBODY_CAUGHT", 0, "Nobody gets busted."))
d.set_editor_property("location_scout", True)
L.set_roles(d, [("Decoy", "North side: let the guard see you, then run a big circle in the north-west until the take ends."),
                ("Pyro", "South side: light the charge by the boarded door and get well clear."),
                ("Hero", "South side: wait out the blast, then through the hole to the goal.")])
L.marks([(1, (-250, 960, 0), (5, 12), (600, -400, 450), "Night chase"),
         (2, (1200, -700, 0), (6, 9), (-500, -200, 300), "Clear of the blast"),
         (3, (2300, -250, 0), (8, 12), (500, 500, 300), "Through the facade")])
L.save_map(MAP)
L.log("Saved " + MAP)
