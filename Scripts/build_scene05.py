"""
Scene05_TheGap graybox (P11, par 3, max 5). Run headless like build_scene02.py.

A "stunt pit" 192 cm deep (= a stunt double's height) and 830 cm long - too wide to jump.
No kill volume in it: you can climb down, but not back up (192 > jump height ~184).
A frozen ghost standing in the pit has its head flush with the floor and becomes a stepping stone.
Intended: take 1 drops in and stands at x = 1550, take 2 drops in and stands at x = 1250,
take 3 hops across their heads (or one long jump onto the far ghost) to the goal.

Layout (cm, X forward): start floor x -600..1000, pit x 1000..1830 (floor at z = -192),
far floor x 1830..3000, goal at x = 2600. Corridor walls y = +/-250.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene05_TheGap"
L.new_map(MAP)

W = 250
PIT_DEPTH = 192
L.box_span("StartFloor", -600, 1000, -W, W, -20, 0)
L.box_span("PitFloor", 1000, 1830, -W, W, -PIT_DEPTH - 20, -PIT_DEPTH)
L.box_span("PitWall_Near", 990, 1000, -W, W, -PIT_DEPTH, 0)
L.box_span("PitWall_Far", 1830, 1840, -W, W, -PIT_DEPTH, 0)
L.box_span("FarFloor", 1830, 3000, -W, W, -20, 0)
L.box_span("Wall_N", -600, 3000, W, W + 30, -PIT_DEPTH - 20, 300)
L.box_span("Wall_S", -600, 3000, -W - 30, -W, -PIT_DEPTH - 20, 300)
L.box_span("Wall_End", 3000, 3030, -W - 30, W + 30, -20, 300)
L.box_span("Wall_Start", -630, -600, -W - 30, W + 30, -20, 300)

# Stunt mats in the pit (visual only), so it reads as a landing zone.
mat = L.box("StuntMat", (1415, 0, -PIT_DEPTH + 5), (800, 2 * W - 20, 10))
L.no_collision(mat)

L.spawn(unreal.GoalZone, (2600, 0, 100), label="GoalZone_FarSide")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((600, -1000, 1200), (1400, 0, -150)), ((2200, 1000, 1200), (1500, 0, -150))])
L.cameras([
    ((-400, -900, 900), (1400, 0, -100), 18),     # wide
    ((1415, -230, 120), (1415, 0, -150), 16),     # in the pit, looking across
    ((2400, 200, 350), (1400, 0, -50), 28),       # far side looking back
    ((1000, 200, 600), (1500, 0, -150), 24),      # above the near edge
])

# Art pass: Tripo props along the walls (corridor is 5 m wide, path runs down the middle).
L.film_crew((-450, 170, 0), -20, (-500, -170, 0), 30,
            lights=[((900, 210, 0), -160), ((1950, -210, 0), 20)],
            extra=[("StageCrate", (2900, 170, 0), 90, 10), ("PlywoodFlat", (2300, 215, 0), 240, 180)])

L.director(start, "Scene 5: The Gap",
           "Mind the gap. A stunt double who stays standing makes a fine stepping stone.",
           par=3, max_takes=5, duration=25,
           bonus=("FINAL_TAKE_UNDER_SECONDS", 15, "Final take in under 15 seconds."))
L.marks([(1, (900, 0, 0), (1.5, 3), (-400, 0, 350), "Over the edge"),
         (2, (500, 0, 0), (4.5, 6.5), (-500, 0, 400), "Run-up"),
         (3, (2250, 0, 0), (10, 14), (500, 0, 350), "Landing")])
# S3 / scene 16: the same wreath under a single spotlight, and the last clapperboard.
L.wreath((2930, -170, 0), 180)
spot = L.spawn(unreal.SpotLight, (2850, -170, 700), label="WreathSpot", folder="Lights")
spot.set_actor_rotation(L.look_at((2850, -170, 700), (2930, -170, 0)), False)
spot.spot_light_component.set_intensity(9000.0)
spot.spot_light_component.set_editor_property("outer_cone_angle", 18.0)
spot.spot_light_component.set_light_color(unreal.LinearColor(1.0, 0.85, 0.65, 1.0))
L.clapperboard((2900, 170, 90), 180, "SCENE 99  -  TAKE 47  -  FINAL")
L.save_map(MAP)
L.log("Saved " + MAP)
