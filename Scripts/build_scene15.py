"""
Scene15_Rehearsal - ST2, reel 4. Short: "just a rehearsal" of the gap jump, onto a safety net that isn't there.
Old mechanic only (the Gap's): a stunt-deep pit you can't climb out of; a take that stays standing in it becomes
a stepping stone. One pit, one stand-in: 2 takes. The empty net frame and the call sheet line (net: MISSING)
carry the story.

Layout (cm): start floor x -600..1000, pit x 1000..1560 (floor at z = -192), far floor x 1560..2600,
goal at (2300, 0). Corridor walls y = +/-250.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene15_Rehearsal"
L.new_map(MAP)
W = 250
PIT_DEPTH = 192
L.box_span("StartFloor", -600, 1000, -W, W, -20, 0)
L.box_span("PitFloor", 1000, 1560, -W, W, -PIT_DEPTH - 20, -PIT_DEPTH)
L.box_span("PitWall_Near", 990, 1000, -W, W, -PIT_DEPTH, 0)
L.box_span("PitWall_Far", 1560, 1570, -W, W, -PIT_DEPTH, 0)
L.box_span("FarFloor", 1560, 2600, -W, W, -20, 0)
L.box_span("Wall_N", -600, 2600, W, W + 30, -PIT_DEPTH - 20, 300)
L.box_span("Wall_S", -600, 2600, -W - 30, -W, -PIT_DEPTH - 20, 300)
L.box_span("Wall_End", 2600, 2630, -W - 30, W + 30, -20, 300)
L.box_span("Wall_Start", -630, -600, -W - 30, W + 30, -20, 300)

L.spawn(unreal.GoalZone, (2300, 0, 100), label="GoalZone_FarSide")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((600, -900, 1100), (1280, 0, -150))])
L.mood(sun_rot=(-60, 60, 0), sun_color=(0.85, 0.85, 0.95), sun_intensity=0.9, sky_intensity=0.2, spot_scale=0.9)

# S3: the net frame with no net in it (posts on the pit's corners, against the walls), and the tag.
for x in (1010, 1550):
    for y in (W - 20, -W + 20):
        L.dressing("NetPost", (x, y, -PIT_DEPTH), (12, 12, PIT_DEPTH + 60), color=(0.5, 0.5, 0.5))
L.dressing("SafetyNet", (1280, W - 10, -60), (540, 4, 4), color=(0.5, 0.5, 0.5))   # only the top cord is left
L.sign("SAFETY NET: MISSING", (1280, W - 6, 170), yaw=-90, size=30, color=(0.95, 0.85, 0.3), label="Sign_NetMissing")

L.cameras([((-400, -800, 800), (1280, 0, -100), 20), ((1280, -230, 120), (1280, 0, -150), 16)])
d = L.director(start, "Scene 15: The Rehearsal",
               "Just a rehearsal of the gap jump. The net never showed up - so somebody has to be the net.",
               par=2, max_takes=4, duration=12, bonus=("NO_SCRAPS", 0, "No scrapped takes."))
L.set_roles(d, [("Net", "Drop into the pit and stand in the middle. Stay standing."),
                ("Stunt", "Run, jump onto the net's head, land on the far side.")])
L.marks([(1, (1280, 0, -PIT_DEPTH), (2.5, 6), (-500, -150, 500), "Where the net should be"),
         (2, (1900, 0, 0), (6, 9.5), (500, -100, 300), "Landing")])
L.wall_clock((-585, 120, 220), 0)  # stopped at 4:47
L.save_map(MAP)
L.log("Saved " + MAP)
