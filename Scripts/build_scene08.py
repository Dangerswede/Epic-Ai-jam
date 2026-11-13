"""
Scene08_BridgeReshoot - ST2 remix of Scene 1 (reel 2 closer). Dusk light; the gate needs 3 takes.
New combination (no new mechanics): ONE gate that only opens while BOTH timed switches are on (require all),
4 s each, and the switches are so far apart that one runner can't keep both on at once.

Layout (cm, X forward) - same studio and bridge as Scene 1:
  Ramp x 200..1000 up to the bridge deck (z = 300), bridge x 1000..2800.
  Gate on the bridge at x = 1800 (require all of A and B).
  Switch A (300, -500) by the ramp, switch B (2600, 700) on the far side, under the other end of the bridge.
  Goal on the far end of the bridge (2650, 0).
Intended: GRIP A and GRIP B time their presses so the windows overlap; HERO crosses in the overlap.
"""
import math
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene08_BridgeReshoot"
L.new_map(MAP)

L.box("StudioFloor", (1500, 0, -10), (6000, 4000, 20))
run, rise = 800.0, 300.0
L.box("Ramp", (600, 0, 140), (math.hypot(run, rise), 300, 20), rot=(math.degrees(math.atan2(rise, run)), 0, 0))
L.box("Bridge", (1900, 0, 290), (1800, 300, 20))
for x in (1100, 1900, 2700):
    for y in (-140, 140):
        L.box("BridgePillar", (x, y, 140), (20, 20, 280))

gate = L.spawn(unreal.Gate, (1800, 0, 300), label="Gate")
switch_a = L.spawn(unreal.TimedSwitch, (300, -500, 55), label="SwitchA")
switch_b = L.spawn(unreal.TimedSwitch, (2600, 700, 55), rot=(0, 180, 0), label="SwitchB")
for sw, name in ((switch_a, "SWITCH A"), (switch_b, "SWITCH B")):
    sw.set_editor_property("display_name", name)
    sw.set_editor_property("active_seconds", 4.0)
gate.set_editor_property("linked_activators", [switch_a, switch_b])
gate.set_editor_property("require_all_activators", True)

L.spawn(unreal.GoalZone, (2650, 0, 400), label="GoalZone")
kill = L.spawn(unreal.KillVolume, (1500, 0, -600), label="KillVolume")
kill.set_actor_scale3d(unreal.Vector(10, 10, 1))
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((800, -1400, 1200), (1900, 0, 0)), ((2400, 1400, 1200), (2200, 600, 0))])
L.mood()  # dusk: low orange sun, dimmer sky and spots
L.cameras([
    ((-700, -1600, 1300), (1500, 0, 150), 18),
    ((900, -700, 600), (1800, 0, 300), 35),
    ((3100, 300, 600), (2300, 0, 300), 28),
])

L.film_crew((-450, -700, 0), 30, (-600, -1000, 0), 60,
            lights=[((600, -1600, 0), 60), ((3200, 1700, 0), -120)],
            extra=[("Scaffolding", (1500, -1700, 0), 600, 0), ("PlywoodFlat", (4100, 1300, 0), 260, 180)])

d = L.director(start, "Scene 8: The Bridge (Reshoot)",
               "Reshoot, and the light is going. ONE gate this time, and it wants BOTH switches on at once - "
               "4 seconds each, too far apart for one runner. Two grips, one hero.",
               par=3, max_takes=5, duration=15, bonus=("FINAL_TAKE_UNDER_SECONDS", 11, "Final take in under 11 seconds."))
L.set_roles(d, [("Grip", "Switch A by the ramp. Don't hit it too early - wait for the far side."),
                ("Best Boy", "Run under the bridge to switch B on the far side and hit it."),
                ("Hero", "Up the ramp and across while both switches hold the gate.")])
L.marks([(1, (300, -330, 0), (3, 5.5), (-500, -500, 350), "Waiting on A"),
         (2, (1550, 670, 0), (3.5, 5.5), (-300, 700, 300), "Under the bridge"),
         (3, (2200, 0, 300), (7.5, 9.5), (500, -700, 250), "Last light")])
L.wall_clock((4085, 1300, 200), 180)  # stopped at 4:47 since scene 7
L.save_map(MAP)
L.log("Saved " + MAP)
