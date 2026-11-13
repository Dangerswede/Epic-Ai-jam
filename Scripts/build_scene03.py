"""
Scene03_Chase graybox - D4 redesign (par 3, max 5, 16 s takes).
New trick: EXPLOSIVES (stunt deaths), combined with an old one: a TIMED SWITCH gate.

Layout (cm, X forward):
  Start room x -600..900. Main corridor x 900..4700 (y -170..170); a parallel NORTH corridor
  x 900..4100 (y 330..670) entered from the start room.
  Charge 1 (fuse 2 s, radius 4 m) at (1600, 110) next to breakable wall 1 (x = 1700).
  Charge 2 (fuse 0.5 s, radius 5 m: whoever lights it takes the fall) at (2900, 110), wall 2 at x = 3000.
  Exit gate at x = 4000, opened for 5 s by the EXIT switch at the far end of the north corridor.
  Goal at x = 4400.
Intended: PYRO lights charge 1, gets clear, then lights charge 2 and is blown up; GRIP runs the north
corridor and hits the exit switch; HERO follows the blasts and makes the exit while it's open.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene03_Chase"
FINAL_N = 18  # director's note: playtest final take (13.8 s) + ~30%
L.new_map(MAP)

W = 170   # main corridor half width (inner)
H = 300

# Floors.
L.box_span("StartFloor", -600, 900, -1000, 1000, -20, 0)
L.box_span("CorridorFloor", 900, 4700, -W, W, -20, 0)
L.box_span("NorthFloor", 900, 4100, 330, 670, -20, 0)

# Start room east wall with two openings: main corridor (y -170..170) and north corridor (y 330..670).
L.box_span("StartWall_E1", 885, 915, 670, 1000, 0, H)
L.box_span("StartWall_E2", 885, 915, W, 330, 0, H)
L.box_span("StartWall_E3", 885, 915, -1000, -W, 0, H)

# Main corridor walls (the north one is thick: it separates the two corridors).
L.box_span("CorridorWall_N", 900, 4700, W, 330, 0, H)
L.box_span("CorridorWall_S", 900, 4700, -W - 30, -W, 0, H)
L.box_span("CorridorWall_End", 4700, 4730, -W - 30, 330, 0, H)
L.box_span("NorthWall_N", 900, 4100, 670, 700, 0, H)
L.box_span("NorthWall_End", 4100, 4130, 330, 700, 0, H)

kill = L.spawn(unreal.KillVolume, (2000, 0, -1200), label="KillVolume_Net")
kill.set_actor_scale3d(unreal.Vector(12, 12, 1))

# Charges and breakable walls.
wall1 = L.box_span("BreakableWall1", 1680, 1720, -W, W, 0, H)
L.tag(wall1, "BreakableWall")
bomb1 = L.spawn(unreal.ExplosiveProp, (1600, 110, 45), label="Charge1")
bomb1.set_editor_property("fuse_seconds", 2.0)
bomb1.set_editor_property("radius", 400.0)
wall2 = L.box_span("BreakableWall2", 2980, 3020, -W, W, 0, H)
L.tag(wall2, "BreakableWall")
bomb2 = L.spawn(unreal.ExplosiveProp, (2900, 110, 45), label="Charge2")
bomb2.set_editor_property("fuse_seconds", 0.5)
bomb2.set_editor_property("radius", 500.0)

# Exit gate + timed switch at the end of the north corridor.
exit_gate = L.spawn(unreal.Gate, (4000, 0, 0), label="ExitGate")
exit_gate.set_actor_scale3d(unreal.Vector(1, 340.0 / 300.0, 1.2))
exit_switch = L.spawn(unreal.TimedSwitch, (3950, 500, 55), label="ExitSwitch")
exit_switch.set_editor_property("display_name", "EXIT")
exit_switch.set_editor_property("active_seconds", 5.0)
exit_gate.set_editor_property("linked_activators", [exit_switch])

L.spawn(unreal.GoalZone, (4400, 0, 100), label="GoalZone_Exit")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((800, -1200, 1400), (1500, 0, 0)), ((3200, 1200, 1400), (3200, 0, 0))])
L.cameras([
    ((-500, 1400, 1600), (2000, 0, 0), 16),
    ((1300, -600, 600), (1650, 0, 120), 28),
    ((2600, -600, 600), (2950, 0, 120), 28),
    ((4600, 0, 250), (3000, 0, 50), 35),
])

# Art pass: Tripo props.
L.set_art(bomb1, "Dynamite")
L.set_art(bomb2, "Dynamite")
L.film_crew((-450, -350, 0), -30, (-500, -700, 0), 30,
            lights=[((750, -850, 0), 150), ((-500, 900, 0), -40)],
            extra=[("Scaffolding", (2500, 950, 0), 600, 0), ("Scaffolding", (3900, -650, 0), 600, 0),
                   ("PlywoodFlat", (700, -950, 0), 260, 90), ("StageCrate", (-450, 500, 0), 90, 15)])

d = L.director(start, "Scene 3: The Chase",
               "New trick: EXPLOSIVES. Blow both walls - the second fuse is short, so whoever lights it takes the fall. "
               "Old trick: the exit is a timed gate. New: the DIRECTOR'S NOTE below is an extra goal for the third star.",
               par=3, max_takes=5, duration=16,
               bonus=("FINAL_TAKE_UNDER_SECONDS", FINAL_N, "Final take in under %d seconds." % FINAL_N))
L.set_roles(d, [("Pyro", "Light charge 1, get clear, then light charge 2 - and take the fall."),
                ("Grip", "Run the north corridor and hit the EXIT switch at the far end."),
                ("Hero", "Follow the blasts and make the exit while the gate is open.")])
L.marks([(1, (1300, 30, 0), (1.5, 3.5), (-500, 0, 250), "Light the fuse"),
         (2, (1900, 500, 0), (4, 6), (-600, 0, 250), "North corridor"),
         (3, (3150, -70, 0), (11, 13.5), (600, 0, 250), "Through the smoke")])
# S3: yesterday's paper on the stage crate in the start room.
L.newspaper((-450, 500, 90), -48)
L.save_map(MAP)
L.log("Saved " + MAP)
