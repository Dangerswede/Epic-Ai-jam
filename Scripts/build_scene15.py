"""
Scene15_Rehearsal - ST2, reel 4. Short: "just a rehearsal" of the gap jump, onto a safety net that isn't there.
Old mechanic only (the Gap's): a pit you can't climb out of; a take that stands still in it becomes
a stepping stone. One pit, one stand-in: 2 takes. The empty net frame and the call sheet line (net: MISSING)
carry the story.

Numbers (stunt double: jump apex ~184 cm, run 500 cm/s, run-jump ~612 cm, +42 cm perch on each edge):
- pit 230 deep: can't climb out (230 > 184 + rounded-edge slack); a standing take's head is 32 cm below the
  floor (head platform top 197.5 cm above its feet), an easy step/jump back up.
- pit 800 long: a straight run-jump needs ~716 cm -> impossible. A take standing in the middle splits it
  into two ~400 cm hops (the second one even from a standstill).

Layout (cm): start floor x -600..1000, pit x 1000..1800 (floor at z = -230), far floor x 1800..2800,
goal at (2500, 0). Corridor walls y = +/-250.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene15_Rehearsal"
L.new_map(MAP)
W = 250
PIT_DEPTH = 230
PIT_END = 1800
L.box_span("StartFloor", -600, 1000, -W, W, -20, 0)
L.box_span("PitFloor", 1000, PIT_END, -W, W, -PIT_DEPTH - 20, -PIT_DEPTH)
L.box_span("PitWall_Near", 990, 1000, -W, W, -PIT_DEPTH, 0)
L.box_span("PitWall_Far", PIT_END, PIT_END + 10, -W, W, -PIT_DEPTH, 0)
L.box_span("FarFloor", PIT_END, 2800, -W, W, -20, 0)
L.box_span("Wall_N", -600, 2800, W, W + 30, -PIT_DEPTH - 20, 300)
L.box_span("Wall_S", -600, 2800, -W - 30, -W, -PIT_DEPTH - 20, 300)
L.box_span("Wall_End", 2800, 2830, -W - 30, W + 30, -20, 300)
L.box_span("Wall_Start", -630, -600, -W - 30, W + 30, -20, 300)

L.spawn(unreal.GoalZone, (2500, 0, 100), label="GoalZone_FarSide")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((700, -900, 1100), (1400, 0, -150))])
L.mood(sun_rot=(-60, 60, 0), sun_color=(0.85, 0.85, 0.95), sun_intensity=0.9, sky_intensity=0.2, spot_scale=0.9)

# S3: the net frame with no net in it (posts on the pit's corners, against the walls), and the tag.
for x in (1010, PIT_END - 10):
    for y in (W - 20, -W + 20):
        L.dressing("NetPost", (x, y, -PIT_DEPTH), (12, 12, PIT_DEPTH + 60), color=(0.5, 0.5, 0.5))
L.dressing("SafetyNet", (1400, W - 10, -60), (780, 4, 4), color=(0.5, 0.5, 0.5))   # only the top cord is left
L.sign("SAFETY NET: MISSING", (1400, W - 6, 170), yaw=-90, size=30, color=(0.95, 0.85, 0.3), label="Sign_NetMissing")

# Tape mark on the pit floor: where the stand-in should stand.
L.dressing("TapeMark", (1400, 0, -PIT_DEPTH + 1), (70, 70, 2), color=(0.95, 0.8, 0.2))

L.cameras([((-400, -800, 800), (1400, 0, -100), 20), ((1400, -230, 120), (1400, 0, -150), 16)])
d = L.director(start, "Scene 15: The Rehearsal",
               "Just a rehearsal of the gap jump. The net never showed up - so somebody has to be the net.",
               par=2, max_takes=4, duration=12, bonus=("FINAL_TAKE_UNDER_SECONDS", 11, "Final take in under 11 seconds."))
L.set_roles(d, [("Net", "Drop into the pit and stand in the middle. Stay standing."),
                ("Stunt", "Jump onto the net's head, then hop to the far side.")])
L.marks([(1, (1400, 0, -PIT_DEPTH), (2.5, 6), (-500, -150, 500), "Where the net should be"),
         (2, (2100, 0, 0), (6, 9.5), (500, -100, 300), "Landing")])
L.wall_clock((-585, 120, 220), 0)  # stopped at 4:47
L.save_map(MAP)
L.log("Saved " + MAP)
