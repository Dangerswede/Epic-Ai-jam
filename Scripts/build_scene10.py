"""
Scene10_EmptyStage - ST2, reel 3. Small and quiet: plates only, tape marks, one chair, the call sheet blown up
on the wall. The director says nothing (story table: no line).
Mechanic (old ones only): the stage door is a gate that needs BOTH pressure plates held. A take that ends on a
plate keeps standing there, so: two takes hold the plates, the third walks through.

Layout (cm): stage x -600..1200, y -900..900, back wall at x = 1200 with a door (gate) in the middle,
small back room x 1200..1800 with the goal. Plates at (600, -650) and (600, 650).
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene10_EmptyStage"
L.new_map(MAP)
H = 320
D = 170  # door half width

L.box_span("StageFloor", -600, 1800, -900, 900, -20, 0)
L.box_span("Wall_N", -600, 1800, 900, 930, 0, H)
L.box_span("Wall_S", -600, 1800, -930, -900, 0, H)
L.box_span("Wall_W", -630, -600, -930, 930, 0, H)
L.box_span("Wall_E", 1800, 1830, -930, 930, 0, H)
L.box_span("BackWall_N", 1185, 1215, D, 900, 0, H)
L.box_span("BackWall_S", 1185, 1215, -900, -D, 0, H)
L.box_span("BackWall_Lintel", 1185, 1215, -D, D, 260, H)

plate_n = L.spawn(unreal.PressurePlate, (600, 650, 40), label="Mark_North")
plate_s = L.spawn(unreal.PressurePlate, (600, -650, 40), label="Mark_South")
door = L.spawn(unreal.Gate, (1200, 0, 0), label="StageDoor")
door.set_actor_scale3d(unreal.Vector(1, 2 * D / 300.0, 260 / 300.0))
door.set_editor_property("linked_activators", [plate_n, plate_s])
door.set_editor_property("require_all_activators", True)

L.spawn(unreal.GoalZone, (1550, 0, 100), label="GoalZone_BackRoom")
start = L.spawn(unreal.PlayerStart, (-300, 0, 100), label="PlayerStart")

# One work light, very little else.
L.studio_lights(sun_rot=(-70, 10, 0), spots=[((300, 0, 1100), (600, 0, 0))])
L.mood(sun_rot=(-70, 10, 0), sun_color=(0.8, 0.85, 1.0), sun_intensity=0.6, sky_intensity=0.15, spot_scale=0.8)

# S3: tape crosses on the floor, one empty director's chair, the call sheet blown up on the wall.
for i, (x, y) in enumerate(((200, 300), (-200, -500), (900, -300), (-400, 600))):
    L.dressing("TapeMark", (x, y, 0), (60, 8, 1), yaw=45, color=(0.9, 0.8, 0.2))
    L.dressing("TapeMark", (x, y, 0), (60, 8, 1), yaw=-45, color=(0.9, 0.8, 0.2))
L.dressing("DirectorChair", (-450, 700, 0), (60, 55, 110), yaw=-30)
L.sign("CALL SHEET\nNOV 14, 1977\n\nSCENE 10\nCREW: NONE\nCAST: NONE\nSTUNT: J. WARD",
       (1180, -520, 190), yaw=180, size=26, label="Sign_CallSheetWall")

L.cameras([((-500, -800, 700), (600, 0, 0), 20), ((1700, 700, 260), (1000, 0, 100), 30)])
d = L.director(start, "Scene 10",
               "The door only opens while both tape marks are taken. Nobody else is here.",
               par=3, max_takes=4, duration=10, bonus=("NO_SCRAPS", 0, "No scrapped takes."))
L.set_roles(d, [("Stand-In", "Stand on the south mark. Stay there."),
                ("Stand-In", "Stand on the north mark. Stay there."),
                ("Stunt", "Walk through the door.")])
L.save_map(MAP)
L.log("Saved " + MAP)
