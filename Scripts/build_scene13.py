"""
Scene13_ProjectionRoom - reel 3 closer. Old pieces: levers + a gate (require all) + camera marks.
The exit needs three levers pulled, spread around a dark projection room, and the takes are short (7 s).
Routing is the puzzle: the obvious answer is one lever per take (3 takes); the tight one is lever A + C in
one take and lever B + the exit in the next (par 2). On the west wall, a projection screen shows the room
from the projector booth - so every earlier take plays on it, again and again.

Layout (cm): room x -800..1200, y -1000..1000. Levers: A (300, 940) north, B (300, -940) south, C (1100, 600).
Exit gate at x = 1200 (door y -170..170), exit room x 1200..1700, goal at (1500, 0).
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene13_ProjectionRoom"
L.new_map(MAP)
W = 170
H = 380

L.box_span("Floor", -800, 1700, -1000, 1000, -20, 0)
L.box_span("Wall_N", -800, 1700, 1000, 1030, 0, H)
L.box_span("Wall_S", -800, 1700, -1030, -1000, 0, H)
L.box_span("Wall_W", -830, -800, -1030, 1030, 0, H)
L.box_span("Wall_E", 1700, 1730, -1030, 1030, 0, H)
L.box_span("ExitWall_N", 1185, 1215, W, 1000, 0, H)
L.box_span("ExitWall_S", 1185, 1215, -1000, -W, 0, H)
L.box_span("ExitLintel", 1185, 1215, -W, W, 260, H)
# (no ceiling: a dark sky does the job and keeps the lighting simple)

lever_a = L.spawn(unreal.Lever, (300, 940, 15), label="LeverA")
lever_b = L.spawn(unreal.Lever, (300, -940, 15), rot=(0, 180, 0), label="LeverB")
lever_c = L.spawn(unreal.Lever, (1100, 600, 15), rot=(0, -90, 0), label="LeverC")
exit_gate = L.spawn(unreal.Gate, (1200, 0, 0), label="ExitGate")
exit_gate.set_actor_scale3d(unreal.Vector(1, 2 * W / 300.0, 260 / 300.0))
exit_gate.set_editor_property("linked_activators", [lever_a, lever_b, lever_c])
exit_gate.set_editor_property("require_all_activators", True)
for lv in (lever_a, lever_b, lever_c):
    L.set_art(lv, "WallLever")

L.spawn(unreal.GoalZone, (1500, 0, 100), label="GoalZone_Exit")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

# Dark room: dim work lights only; the screen is the brightest thing in it.
L.studio_lights(sun_rot=(-80, 0, 0), spots=[((300, 0, 360), (300, 0, 0)), ((1100, 400, 360), (1100, 600, 0))])
L.mood(sun_rot=(-80, 0, 0), sun_color=(0.6, 0.6, 0.7), sun_intensity=0.2, sky_intensity=0.3, spot_scale=0.7)

screen = L.spawn(unreal.ProjectionScreen, (-790, 0, 200), label="ProjectionScreen", folder="Dressing")
screen.set_editor_property("capture_from", unreal.Vector(1050, -860, 300))
screen.set_editor_property("capture_at", unreal.Vector(200, 100, 0))
L.dressing("Projector", (1150, -930, 300), (60, 40, 40), yaw=150, color=(0.1, 0.1, 0.1))
L.wall_clock((1685, 600, 230), 180)  # stopped at 4:47
L.director_chairs((-600, -800, 0), yaw=150, n=1)

L.cameras([((-700, -900, 340), (500, 0, 0), 22), ((1650, 900, 300), (800, 0, 50), 26)])
d = L.director(start, "Scene 13: The Projection Room",
               "The exit wants all three levers. Takes are short. Route it well and two takes will do.",
               par=2, max_takes=5, duration=7, bonus=("FINAL_TAKE_UNDER_SECONDS", 6, "Final take in under 6 seconds."))
L.set_roles(d, [("Projectionist", "Lever A on the north wall, then lever C by the exit."),
                ("Stunt", "Lever B on the south wall, then out through the exit.")])
L.marks([(1, (300, 820, 0), (1, 3), (-500, -300, 300), "Lever A"),
         (2, (1050, 450, 0), (3, 5), (-400, -500, 300), "Lever C"),
         (3, (300, -820, 0), (1, 3), (-500, 400, 300), "Lever B")])
L.save_map(MAP)
L.log("Saved " + MAP)
