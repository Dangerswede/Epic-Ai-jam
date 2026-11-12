"""
Scene03_Chase graybox (3-4 takes). Run headless like build_scene02.py.

Layout (cm, X forward), escape corridor inner width 340 (y -170..170), walls 300 tall:
  Start room x -600..900. Pressure plate at (200, -700) opens the gate at x = 1000.
  Explosive (short fuse 0.5 s, radius 5 m: whoever arms it doesn't get away) at x = 1600,
  next to the breakable wall at x = 1700.
  Long crate (780 x 280 x 150) at x 2000..2780. Pit x 2800..3600 (too wide to jump), with
  crate-only rails at z = -150 so the pushed crate lands flush with the floor. Goal at x = 4300.
  Intended: take 1 holds the plate, take 2 arms the explosive (and is blown up - stunt!),
  take 3 waits, then pushes the crate into the pit and escapes.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene03_Chase"
FINAL_N = 20  # director's note: playtest final take + ~30%
L.new_map(MAP)

W = 170   # corridor half width (inner)
H = 300

# Floors.
L.box_span("StartFloor", -600, 900, -1000, 1000, -20, 0)
L.box_span("CorridorFloorA", 900, 2800, -W, W, -20, 0)
L.box_span("CorridorFloorB", 3600, 4700, -W, W, -20, 0)
L.box_span("StartWall_E1", 885, 915, W, 1000, 0, H)
L.box_span("StartWall_E2", 885, 915, -1000, -W, 0, H)

# Corridor walls (span the pit too).
L.box_span("CorridorWall_N", 900, 4700, W, W + 30, -400, H)
L.box_span("CorridorWall_S", 900, 4700, -W - 30, -W, -400, H)
L.box_span("CorridorWall_End", 4700, 4730, -W - 30, W + 30, 0, H)

# Pit rails: block the crate's floor traces, ignore pawns (players fall through).
for i, (y0, y1) in enumerate(((130, W), (-W, -130))):
    rail = L.box_span("PitRail%d" % (i + 1), 2800, 3600, y0, y1, -170, -150, folder="Pit")
    L.ignore_pawns(rail)

kill = L.spawn(unreal.KillVolume, (3200, 0, -600), label="KillVolume_Pit")
kill.set_actor_scale3d(unreal.Vector(4, 4, 1))
kill2 = L.spawn(unreal.KillVolume, (2000, 0, -1200), label="KillVolume_Net")
kill2.set_actor_scale3d(unreal.Vector(12, 12, 1))

# Gate + plate.
plate = L.spawn(unreal.PressurePlate, (200, -700, 40), label="PressurePlate")
gate = L.spawn(unreal.Gate, (1000, 0, 0), label="Gate")
gate.set_actor_scale3d(unreal.Vector(1, 340.0 / 300.0, 1.2))
gate.set_editor_property("linked_activators", [plate])

# Breakable wall + explosive.
wall = L.box_span("BreakableWall", 1680, 1720, -W, W, 0, H)
L.tag(wall, "BreakableWall")
bomb = L.spawn(unreal.ExplosiveProp, (1600, 110, 45), label="Explosive")
bomb.set_editor_property("fuse_seconds", 0.5)
bomb.set_editor_property("radius", 500.0)

# Long crate.
crate = L.spawn(unreal.PushCrate, (2390, 0, 75), label="BridgeCrate")
crate.set_editor_property("size", unreal.Vector(780, 280, 150))
crate.set_actor_scale3d(unreal.Vector(7.8, 2.8, 1.5))

L.spawn(unreal.GoalZone, (4350, 0, 100), label="GoalZone_Exit")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((800, -1200, 1400), (1500, 0, 0)), ((3200, 1200, 1400), (3200, 0, 0))])
L.cameras([
    ((-500, 1400, 1600), (2000, 0, 0), 16),      # wide overview
    ((1300, -600, 600), (1650, 0, 120), 28),     # explosive / wall (looking over the wall)
    ((2600, 0, 900), (3200, 0, -100), 24),       # pit from above
    ((4600, 0, 250), (3000, 0, 50), 35),         # exit looking back
])

L.director(start, "Scene 3: The Chase",
           "Big finale energy. Blow the wall, bridge the gap, get out.",
           par=3, max_takes=6, duration=30,
           bonus=("FINAL_TAKE_UNDER_SECONDS", FINAL_N, "Final take in under %d seconds." % FINAL_N))
L.save_map(MAP)
L.log("Saved " + MAP)
