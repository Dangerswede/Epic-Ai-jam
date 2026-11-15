"""
Scene05_TheGap graybox (P11, par 3, max 5). Run headless like build_scene02.py.

A "stunt pit" 230 cm deep and 1500 cm long. No kill volume in it: you can climb down, but not back up
(230 > jump apex ~184 + rounded-edge slack). A take standing still in the pit has its head 32 cm below the
floor (head platform top 197.5 cm above its feet) and becomes a stepping stone.

Why 1500 (stunt double: run-jump ~612 cm on the level, ~640 down onto a head, ~584 up from a head,
+42 cm perch on each edge): with ONE stand-in the best spot still leaves a hop > 626 cm somewhere
(edge->head reach ~779, head->edge ~626, 779 + 626 < 1500 - margin) -> needs TWO stand-ins.
Intended: take 1 stands at x = 2060 (far), take 2 at x = 1560 (near), take 3 hops edge -> 1560 -> 2060 -> edge
(hops 560 / 500 / 440 cm).

Layout (cm, X forward): start floor x -600..1000, pit x 1000..2500 (floor at z = -230),
far floor x 2500..3700, goal at x = 3300. Corridor walls y = +/-250.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene05_TheGap"
L.new_map(MAP)

W = 250
PIT_DEPTH = 230
PIT_END = 2500
END = 3700
L.box_span("StartFloor", -600, 1000, -W, W, -20, 0)
L.box_span("PitFloor", 1000, PIT_END, -W, W, -PIT_DEPTH - 20, -PIT_DEPTH)
L.box_span("PitWall_Near", 990, 1000, -W, W, -PIT_DEPTH, 0)
L.box_span("PitWall_Far", PIT_END, PIT_END + 10, -W, W, -PIT_DEPTH, 0)
L.box_span("FarFloor", PIT_END, END, -W, W, -20, 0)
L.box_span("Wall_N", -600, END, W, W + 30, -PIT_DEPTH - 20, 300)
L.box_span("Wall_S", -600, END, -W - 30, -W, -PIT_DEPTH - 20, 300)
L.box_span("Wall_End", END, END + 30, -W - 30, W + 30, -20, 300)
L.box_span("Wall_Start", -630, -600, -W - 30, W + 30, -20, 300)

# Stunt mats in the pit (visual only), so it reads as a landing zone.
mat = L.box("StuntMat", (1750, 0, -PIT_DEPTH + 5), (1470, 2 * W - 20, 10))
L.no_collision(mat)
# Tape marks on the mat: where the two stand-ins should stand.
for x in (1560, 2060):
    L.dressing("TapeMark", (x, 0, -PIT_DEPTH + 11), (70, 70, 2), color=(0.95, 0.8, 0.2))

L.spawn(unreal.GoalZone, (3300, 0, 100), label="GoalZone_FarSide")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((900, -1000, 1200), (1550, 0, -150)), ((2700, 1000, 1200), (2050, 0, -150))])
L.cameras([
    ((-400, -1100, 1100), (1750, 0, -100), 18),   # wide
    ((1800, -230, 120), (1800, 0, -150), 16),     # in the pit, looking across
    ((3100, 200, 350), (1800, 0, -50), 28),       # far side looking back
    ((1000, 200, 600), (1800, 0, -150), 24),      # above the near edge
])

# Art pass: Tripo props along the walls (corridor is 5 m wide, path runs down the middle).
L.film_crew((-450, 170, 0), -20, (-500, -170, 0), 30,
            lights=[((900, 210, 0), -160), ((2650, -210, 0), 20)],
            extra=[("StageCrate", (3600, 170, 0), 90, 10), ("PlywoodFlat", (3000, 215, 0), 240, 180)])

L.director(start, "Scene 5: The Gap",
           "Mind the gap. A stunt double who stays standing makes a fine stepping stone.",
           par=3, max_takes=5, duration=25,
           bonus=("FINAL_TAKE_UNDER_SECONDS", 15, "Final take in under 15 seconds."))
L.marks([(1, (900, 0, 0), (1.5, 3), (-400, 0, 350), "Over the edge"),
         (2, (500, 0, 0), (4.5, 6.5), (-500, 0, 400), "Run-up"),
         (3, (2950, 0, 0), (10, 14), (500, 0, 350), "Landing")])
# S3 / scene 16: the same wreath under a single spotlight, and the last clapperboard.
L.wreath((END - 70, -170, 0), 180)
spot = L.spawn(unreal.SpotLight, (END - 150, -170, 700), label="WreathSpot", folder="Lights")
spot.set_actor_rotation(L.look_at((END - 150, -170, 700), (END - 70, -170, 0)), False)
spot.spot_light_component.set_intensity(9000.0)
spot.spot_light_component.set_editor_property("outer_cone_angle", 18.0)
spot.spot_light_component.set_light_color(unreal.LinearColor(1.0, 0.85, 0.65, 1.0))
L.clapperboard((END - 100, 170, 90), 180, "SCENE 99  -  TAKE 47  -  FINAL")
L.wall_clock((END - 15, 0, 230), 180)  # stopped at 4:47
L.save_map(MAP)
L.log("Saved " + MAP)
