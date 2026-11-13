"""
Scene05_Rooftop - reel 2 (cut-line order: first extra scene). Short, 2 takes. Old pieces: lasers + a timed switch.
A rooftop set split by a wall with a laser door. The switch that kills the lasers (3.5 s) is in the far back
corner - one runner can't hit it and make the door. Coffee is still steaming on the crew table; nobody is there.

Layout (cm): roof x -600..2800, y -700..700, low parapet. Wall at x = 1400 with a door (y -170..170) and
3 lasers. Switch at (-530, -630). Goal at (2400, 0). Crew table at (-300, 520) - off the path.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene05_Rooftop"
L.new_map(MAP)
W = 170
H = 300

L.box_span("Roof", -600, 2800, -700, 700, -20, 0)
for name, span in (("Parapet_N", (-600, 2800, 700, 730)), ("Parapet_S", (-600, 2800, -730, -700)),
                   ("Parapet_W", (-630, -600, -730, 730)), ("Parapet_E", (2800, 2830, -730, 730))):
    L.box_span(name, span[0], span[1], span[2], span[3], 0, 100)
L.box_span("DoorWall_N", 1385, 1415, W, 700, 0, H)
L.box_span("DoorWall_S", 1385, 1415, -700, -W, 0, H)
L.box_span("DoorLintel", 1385, 1415, -W, W, 230, H)
kill = L.spawn(unreal.KillVolume, (1100, 0, -1500), label="KillVolume_Street")
kill.set_actor_scale3d(unreal.Vector(20, 20, 1))

switch = L.spawn(unreal.TimedSwitch, (-530, -630, 55), rot=(0, 45, 0), label="LaserSwitch")
switch.set_editor_property("display_name", "LASERS")
switch.set_editor_property("active_seconds", 3.5)
for i, z in enumerate((40, 110, 180)):
    laser = L.spawn(unreal.LaserBeam, (1400, -W, z), rot=(0, 90, 0), label="Laser%d" % (i + 1))
    laser.set_editor_property("beam_length", 2.0 * W)
    laser.set_editor_property("linked_activators", [switch])

L.spawn(unreal.GoalZone, (2400, 0, 100), label="GoalZone_Edge")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(sun_rot=(-40, 140, 0), spots=[])
L.mood(sun_rot=(-40, 140, 0), sun_color=(1.0, 0.92, 0.8), sun_intensity=3.0, sky_intensity=0.6, spot_scale=1.0)

# S3: the crew table - coffee still steaming, nobody there.
L.coffee((-300, 520, 0))
for dx, dy in ((-18, -12), (16, 10), (2, 20)):
    if dx != 0:
        L.dressing("CoffeeCup", (-300 + dx, 520 + dy, 75), (9, 9, 12))
    L.spawn(unreal.SteamWisp, (-300 + dx, 520 + dy, 88), label="Steam", folder="Dressing")
L.director_chairs((-480, 380, 0), yaw=-60)
L.clapperboard((-150, 620, 0), -90, "RETAKE  -  NOV 14 1977")

L.cameras([((-500, -650, 600), (1400, 0, 50), 20), ((2700, 600, 300), (1400, 0, 100), 28)])
d = L.director(start, "Scene 5: The Rooftop",
               "Lasers across the roof door, and their switch is in the far corner - too far for one runner. "
               "The crew left their coffee. Nobody's drinking it.",
               par=2, max_takes=4, duration=12, bonus=("FINAL_TAKE_UNDER_SECONDS", 7, "Final take in under 7 seconds."))
L.set_roles(d, [("Tech", "Back corner: hit the LASERS switch."),
                ("Hero", "Straight through the laser door while it's dark, to the edge.")])
L.marks([(1, (-450, -550, 0), (1, 3), (500, 300, 300), "The switch"),
         (2, (1700, 0, 0), (2.5, 5), (600, -500, 300), "Through the door")])
L.save_map(MAP)
L.log("Saved " + MAP)
