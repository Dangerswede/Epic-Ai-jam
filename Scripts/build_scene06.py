"""
Scene06_Saloon - reel 2. Old pieces: two guards + a push crate. 2 takes, 16 s.
A western saloon set. The goal is up on the balcony along the east wall (250 cm: too high to jump from the floor).
Two guards walk the floor. Take 1 (DECOY) gets spotted by BOTH and leads them round a big circle in the west half -
two slower chasers can't close on a circle either. Take 2 (STUNT) pushes the crate against the balcony and climbs
crate -> balcony. A poster on the wall: WANTED: ONE MORE TAKE.

Layout (cm): saloon x -1200..2600, y -1200..1200. Balcony x 2200..2600 (top z = 250). Crate (150) at (1500, 600).
Guards start at (1200, +/-400) facing west, walking to x = 600. Start in the NW corner. Decoy circle centre (-350, -300), R 600.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene06_Saloon"
L.new_map(MAP)
H = 420

L.box_span("Floor", -1200, 2600, -1200, 1200, -20, 0)
L.box_span("Wall_N", -1200, 2600, 1200, 1230, 0, H)
L.box_span("Wall_S", -1200, 2600, -1230, -1200, 0, H)
L.box_span("Wall_W", -1230, -1200, -1230, 1230, 0, H)
L.box_span("Wall_E", 2600, 2630, -1230, 1230, 0, H)
L.box_span("Balcony", 2200, 2600, -1200, 1200, 0, 250)
L.box_span("BalconyRail", 2580, 2600, -1200, 1200, 250, 340)
L.box_span("BarCounter", 300, 1500, 1060, 1180, 0, 110)  # along the north wall, off every route

crate = L.spawn(unreal.PushCrate, (1500, 600, 75), label="SaloonCrate")
crate.set_editor_property("size", unreal.Vector(150, 150, 150))
crate.set_actor_scale3d(unreal.Vector(1.5, 1.5, 1.5))
L.set_art(crate, "StageCrate")

for i, y in enumerate((400, -400)):
    g = L.spawn(unreal.StuntGuard, (1200, y, 100), rot=(0, 180, 0), label="Guard%d" % (i + 1))
    g.set_editor_property("patrol_point_b", unreal.Vector(600, 0, 0))  # yaw 180: walks west first
    g.set_editor_property("lose_sight_multiplier", 3.0)

L.spawn(unreal.GoalZone, (2400, 600, 350), label="GoalZone_Balcony")
start = L.spawn(unreal.PlayerStart, (-800, 900, 100), rot=(0, -30, 0), label="PlayerStart")  # NW corner, behind the action

L.studio_lights(sun_rot=(-35, 200, 0), spots=[((300, -900, 1200), (0, 0, 0)), ((1800, 900, 1200), (2000, 600, 0))])
L.mood(sun_rot=(-35, 200, 0), sun_color=(1.0, 0.8, 0.55), sun_intensity=2.2, sky_intensity=0.4, spot_scale=0.9)

# S3: the poster, and the crew's chairs by the door.
L.sign("WANTED\nONE MORE TAKE", (-1185, -300, 220), yaw=0, size=46, color=(0.35, 0.18, 0.08), label="Sign_Wanted")
L.dressing("Poster", (-1195, -300, 130), (4, 200, 190), color=(0.85, 0.75, 0.55))
L.director_chairs((-1050, 450, 0), yaw=0)
L.clapperboard((-1080, 250, 0), 0, "RETAKE  -  NOV 14 1977")

L.cameras([((-1100, -1100, 900), (700, 200, 0), 18), ((2500, -900, 600), (2000, 500, 150), 26)])
d = L.director(start, "Scene 6: The Saloon",
               "Two guards on the saloon floor, and the goal is up on the balcony - too high to jump. "
               "One decoy can keep BOTH guards busy if the circle is big enough.",
               par=2, max_takes=4, duration=16, bonus=("NOBODY_CAUGHT", 0, "Nobody gets busted."))
L.set_roles(d, [("Decoy", "Let both guards see you, then run a big circle in the west half until the take ends."),
                ("Stunt", "Push the crate to the balcony, climb up, reach the goal.")])
L.marks([(1, (-350, -900, 0), (5, 13), (500, 500, 450), "Two on one"),
         (2, (1900, 600, 0), (4, 9), (-600, -500, 350), "The crate")])
L.save_map(MAP)
L.log("Saved " + MAP)
