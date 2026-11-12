"""
Scene04_Finale graybox (par 4, max 6). Run headless like build_scene02.py.

Layout (cm, X forward), one long run from the sound stage to a rooftop:
  Start room x -600..900. Plate (200, -700) opens Gate at x = 1000 (corridor y -170..170).
  Explosive (fuse 0.5 s, radius 5 m) at (1600, 110) next to the breakable wall at x = 1700.
  Courtyard x 2000..4400, y -1500..1500. Guard patrols x 2600..3300 at y = 700, watching the lever (2900, 1100).
  East courtyard wall has a door at x = 4400 guarded by 3 lasers (linked to the lever).
  Back alley x 4400..6300 (y -170..170): long crate x 4620..5380, pit x 5400..6200 (crate-only rails).
  Ramp x 6300..7100 up to the rooftop (z = 300), goal at (7600, 0).
  Intended: 1 holds the plate, 2 arms the explosive (and is blown up), 3 lures the guard,
  4 pulls the lever, pushes the crate into the pit and runs up to the roof.
"""
import math
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene04_Finale"
L.new_map(MAP)

W = 170
H = 300

# --- Sound stage (start) + corridor ---------------------------------------------------------
L.box_span("StartFloor", -600, 900, -1000, 1000, -20, 0)
L.box_span("StartWall_E1", 885, 915, W, 1000, 0, H)
L.box_span("StartWall_E2", 885, 915, -1000, -W, 0, H)
L.box_span("CorridorFloor", 900, 2000, -W, W, -20, 0)
L.box_span("CorridorWall_N", 900, 2000, W, W + 30, 0, H)
L.box_span("CorridorWall_S", 900, 2000, -W - 30, -W, 0, H)

plate = L.spawn(unreal.PressurePlate, (200, -700, 40), label="PressurePlate")
gate = L.spawn(unreal.Gate, (1000, 0, 0), label="Gate")
gate.set_actor_scale3d(unreal.Vector(1, 340.0 / 300.0, 1.2))
gate.set_editor_property("linked_activators", [plate])

wall = L.box_span("BreakableWall", 1680, 1720, -W, W, 0, H)
L.tag(wall, "BreakableWall")
bomb = L.spawn(unreal.ExplosiveProp, (1600, 110, 45), label="Explosive")
bomb.set_editor_property("fuse_seconds", 0.5)
bomb.set_editor_property("radius", 500.0)

# --- Courtyard: guard + lever, laser door ---------------------------------------------------
L.box_span("CourtyardFloor", 2000, 4400, -1500, 1500, -20, 0)
L.box_span("CourtyardWall_N", 2000, 4400, 1500, 1530, 0, H)
L.box_span("CourtyardWall_S", 2000, 4400, -1530, -1500, 0, H)
L.box_span("CourtyardWall_W1", 1985, 2015, W + 30, 1530, 0, H)
L.box_span("CourtyardWall_W2", 1985, 2015, -1530, -W - 30, 0, H)
L.box_span("CourtyardWall_E1", 4385, 4415, W, 1530, 0, H)
L.box_span("CourtyardWall_E2", 4385, 4415, -1530, -W, 0, H)
L.box_span("LaserDoorLintel", 4385, 4415, -W, W, 230, H)
L.box("CourtyardCrate1", (4050, 1200, 60), (120, 120, 120))
L.box("CourtyardCrate2", (2400, -1100, 90), (180, 180, 180))

lever = L.spawn(unreal.Lever, (2900, 1100, 15), label="Lever")
for i, z in enumerate((40, 110, 180)):
    laser = L.spawn(unreal.LaserBeam, (4400, -W, z), rot=(0, 90, 0), label="Laser%d" % (i + 1))
    laser.set_editor_property("beam_length", 340.0)
    laser.set_editor_property("linked_activators", [lever])

guard = L.spawn(unreal.StuntGuard, (2600, 700, 100), label="Guard")
guard.set_editor_property("patrol_point_b", unreal.Vector(700, 0, 0))
guard.set_editor_property("lose_sight_multiplier", 3.0)

# --- Back alley: crate + pit ----------------------------------------------------------------
L.box_span("AlleyFloorA", 4400, 5400, -W, W, -20, 0)
L.box_span("AlleyFloorB", 6200, 6300, -W, W, -20, 0)
L.box_span("AlleyWall_N", 4400, 6300, W, W + 30, -400, H)
L.box_span("AlleyWall_S", 4400, 6300, -W - 30, -W, -400, H)
for i, (y0, y1) in enumerate(((130, W), (-W, -130))):
    rail = L.box_span("PitRail%d" % (i + 1), 5400, 6200, y0, y1, -170, -150, folder="Pit")
    L.ignore_pawns(rail)
crate = L.spawn(unreal.PushCrate, (5000, 0, 75), label="BridgeCrate")
crate.set_editor_property("size", unreal.Vector(760, 280, 150))
crate.set_actor_scale3d(unreal.Vector(7.6, 2.8, 1.5))

# --- Ramp + rooftop -------------------------------------------------------------------------
run, rise = 800.0, 300.0
L.box("RoofRamp", (6700, 0, 150 - 10), (math.hypot(run, rise), 340, 20), rot=(math.degrees(math.atan2(rise, run)), 0, 0))
L.box_span("RampWall_N", 6300, 7100, W, W + 30, 0, 450)
L.box_span("RampWall_S", 6300, 7100, -W - 30, -W, 0, 450)
L.box_span("Building", 7100, 7900, -500, 500, -20, 300)
for name, (x0, x1, y0, y1) in {
    "Parapet_N": (7100, 7900, 470, 500), "Parapet_S": (7100, 7900, -500, -470), "Parapet_E": (7870, 7900, -500, 500),
}.items():
    L.box_span(name, x0, x1, y0, y1, 300, 400)
L.box("WaterTower", (7700, 350, 550), (150, 150, 300), mesh=unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder"))
L.box("Billboard", (7500, -480, 600), (500, 20, 250))
L.spawn(unreal.GoalZone, (7600, 0, 400), label="GoalZone_Rooftop")

# --- Safety nets ----------------------------------------------------------------------------
kill = L.spawn(unreal.KillVolume, (5800, 0, -600), label="KillVolume_Pit")
kill.set_actor_scale3d(unreal.Vector(4, 4, 1))
net = L.spawn(unreal.KillVolume, (4000, 0, -1200), label="KillVolume_Net")
net.set_actor_scale3d(unreal.Vector(20, 10, 1))

start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((1500, -1600, 1500), (1800, 0, 0)), ((3300, 1600, 1500), (3000, 600, 0))])
L.cameras([
    ((-400, -2200, 2400), (3500, 0, 0), 14),     # big wide over the whole set
    ((2300, -1300, 500), (2900, 900, 100), 28),  # courtyard: guard + lever
    ((4900, -900, 900), (5800, 0, -50), 24),     # alley + pit (looking over the south wall)
    ((8300, -900, 900), (7300, 0, 350), 30),     # rooftop finale
])

L.director(start, "Scene 4: The Finale",
           "Final scene. Everything you've learned. Make it a classic.",
           par=4, max_takes=6, duration=45)
L.save_map(MAP)
L.log("Saved " + MAP)
