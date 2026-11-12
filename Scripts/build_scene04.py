"""
Scene04_Finale graybox (par 4, max 7). Run headless like build_scene02.py.

Layout (cm, X forward), one long run from the sound stage to a rooftop:
  Start room x -600..900. TWO plates (200, 700) and (200, -700) must BOTH be pressed to open the
  gate at x = 1000 (corridor y -170..170). A small crate (150 cm) at (200, 300) can be pushed onto
  the north plate - crates press plates too - so one take can hold both marks.
  Courtyard x 2000..4400, y -1500..1500. Guard patrols x 2600..3300 at y = 700, watching the lever (2900, 1100).
  East courtyard wall has a door at x = 4400 guarded by 3 lasers (linked to the lever).
  Back alley x 4400..6300 (y -170..170): long crate x 4620..5380, pit x 5400..6200 (crate-only rails).
  Ramp x 6300..7100 up to the rooftop (z = 300), goal at (7600, 0).
  Intended (par 4): plate A, plate B, lure the guard, then lever + crate bridge + roof.
  The crate trick (push the small crate onto plate A, stand on plate B) saves a take: 3 takes.
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

plate_n = L.spawn(unreal.PressurePlate, (200, 700, 40), label="PressurePlate_North")
plate_s = L.spawn(unreal.PressurePlate, (200, -700, 40), label="PressurePlate_South")
gate = L.spawn(unreal.Gate, (1000, 0, 0), label="Gate")
gate.set_actor_scale3d(unreal.Vector(1, 340.0 / 300.0, 1.2))
gate.set_editor_property("linked_activators", [plate_n, plate_s])
gate.set_editor_property("require_all_activators", True)

prop_crate = L.spawn(unreal.PushCrate, (200, 300, 75), label="PropCrate")
prop_crate.set_editor_property("size", unreal.Vector(150, 150, 150))
prop_crate.set_actor_scale3d(unreal.Vector(1.5, 1.5, 1.5))

# --- Courtyard: guard + lever, laser door ---------------------------------------------------
L.box_span("CourtyardFloor", 2000, 4400, -1500, 1500, -20, 0)
L.box_span("CourtyardWall_N", 2000, 4400, 1500, 1530, 0, H)
L.box_span("CourtyardWall_S", 2000, 4400, -1530, -1500, 0, H)
L.box_span("CourtyardWall_W1", 1985, 2015, W + 30, 1530, 0, H)
L.box_span("CourtyardWall_W2", 1985, 2015, -1530, -W - 30, 0, H)
L.box_span("CourtyardWall_E1", 4385, 4415, W, 1530, 0, H)
L.box_span("CourtyardWall_E2", 4385, 4415, -1530, -W, 0, H)
L.box_span("LaserDoorLintel", 4385, 4415, -W, W, 230, H)
court1 = L.box("CourtyardCrate1", (4050, 1200, 60), (120, 120, 120))
court2 = L.box("CourtyardCrate2", (2400, -1100, 90), (180, 180, 180))

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
    ((-500, 0, 500), (300, 0, 50), 20),          # start room: both marks + prop crate
    ((2300, -1300, 500), (2900, 900, 100), 28),  # courtyard: guard + lever
    ((4900, -900, 900), (5800, 0, -50), 24),     # alley + pit (looking over the south wall)
    ((8300, -900, 900), (7300, 0, 350), 30),     # rooftop finale
])

# Art pass: Tripo props (keep the courtyard's south side free - the lure runs there).
L.set_art(plate_n, "PressurePlate")
L.set_art(plate_s, "PressurePlate")
L.set_art(prop_crate, "StageCrate")
L.set_art(lever, "WallLever")
L.overlay_static(court1, "StageCrate")
L.overlay_static(court2, "StageCrate")
L.film_crew((-450, -350, 0), 20, (-500, 400, 0), -40,
            lights=[((-500, 900, 0), -50), ((-500, -900, 0), 50), ((7850, -420, 300), 140)],
            extra=[("Scaffolding", (3600, 1350, 0), 600, 0), ("Scaffolding", (2200, 1350, 0), 600, 0),
                   ("PlywoodFlat", (650, 950, 0), 260, 180), ("DirectorChair", (7400, -380, 300), 100, 120),
                   ("Clapperboard", (7300, 380, 300), 40, -30), ("CinemaCamera", (7200, 300, 300), 170, -60)])

L.director(start, "Scene 4: The Finale",
           "Two marks, one gate. Props can hit a mark too.",
           par=4, max_takes=7, duration=45,
           bonus=("NO_SCRAPS", 0, "No scrapped takes."))
L.save_map(MAP)
L.log("Saved " + MAP)
