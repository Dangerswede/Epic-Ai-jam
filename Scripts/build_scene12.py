"""
Scene12_TheRig - ST2, reel 3. Old pieces only: push crate + timed switch + kill volume.
A long crate has to be pushed into a pit to bridge it (the pit is a drop into a kill volume), and the exit gate
on the far side is held open for 4 s by a switch back in the start room - too far for the hero to hit it and
make the gate. Dressing: a lighting rig over the pit with a tag that says NOT INSPECTED.

Layout (cm, X forward): start room x -400..1400, y -800..800. Corridor y -170..170 from x 1400.
  Crate 760 x 280 x 150 at (1000, 0). Pit x 1400..2200 (crate-only rails at z -170..-150), kill volume below.
  Floor B x 2200..3700, exit gate at x 3000, goal at (3450, 0). Switch at (300, -650).
Intended: RIGGER pushes the crate into the pit; GRIP hits the switch late; HERO crosses on the crate, through the gate.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene12_TheRig"
L.new_map(MAP)
W = 170
H = 320

L.box_span("RoomFloor", -400, 1400, -800, 800, -20, 0)
L.box_span("Room_N", -400, 1400, 800, 830, 0, H)
L.box_span("Room_S", -400, 1400, -830, -800, 0, H)
L.box_span("Room_W", -430, -400, -830, 830, 0, H)
L.box_span("Room_E1", 1385, 1415, W, 830, 0, H)
L.box_span("Room_E2", 1385, 1415, -830, -W, 0, H)
L.box_span("FloorB", 2200, 3700, -W, W, -20, 0)
L.box_span("Corr_N", 1400, 3700, W, W + 30, -400, H)
L.box_span("Corr_S", 1400, 3700, -W - 30, -W, -400, H)
L.box_span("Corr_End", 3700, 3730, -W - 30, W + 30, 0, H)
for i, (y0, y1) in enumerate(((130, W), (-W, -130))):
    rail = L.box_span("PitRail%d" % (i + 1), 1400, 2200, y0, y1, -170, -150, folder="Pit")
    L.ignore_pawns(rail)
kill = L.spawn(unreal.KillVolume, (1800, 0, -600), label="KillVolume_Pit")
kill.set_actor_scale3d(unreal.Vector(10, 4, 1))

crate = L.spawn(unreal.PushCrate, (1000, 0, 75), label="RigCrate")
crate.set_editor_property("size", unreal.Vector(760, 280, 150))
crate.set_actor_scale3d(unreal.Vector(7.6, 2.8, 1.5))

gate = L.spawn(unreal.Gate, (3000, 0, 0), label="ExitGate")
gate.set_actor_scale3d(unreal.Vector(1, 2 * W / 300.0, 1))
switch = L.spawn(unreal.TimedSwitch, (300, -650, 55), label="ExitSwitch")
switch.set_editor_property("display_name", "EXIT")
switch.set_editor_property("active_seconds", 4.0)
gate.set_editor_property("linked_activators", [switch])

L.spawn(unreal.GoalZone, (3450, 0, 100), label="GoalZone_Exit")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((600, -1200, 1300), (1000, 0, 0)), ((2200, 1000, 1300), (1800, 0, -100))])
L.mood(sun_rot=(-35, 120, 0), sun_color=(0.75, 0.8, 1.0), sun_intensity=1.2, sky_intensity=0.2, spot_scale=0.7)

# S3: the rig over the pit (never in the path: up on the corridor walls) with its tag.
L.dressing("StudioLight", (1800, 0, H + 20), (900, 60, 40), color=(0.15, 0.15, 0.15))
L.sign("RIG: NOT INSPECTED", (1800, -W - 40, 230), yaw=90, size=28, color=(0.95, 0.2, 0.15), label="Sign_RigTag")
L.sign("RIG: NOT INSPECTED", (1390, -500, 200), yaw=180, size=24, color=(0.95, 0.2, 0.15), label="Sign_RigTag2")
L.set_art(crate, "StageCrate")

L.cameras([((-300, -700, 700), (900, 0, 0), 20), ((1500, -900, 900), (1900, 0, -50), 24), ((3650, 0, 260), (2600, 0, 50), 30)])
d = L.director(start, "Scene 12: The Rig",
               "Bridge the drop with the long crate. The exit is a timed gate, and its switch is back here - "
               "too far for one runner.",
               par=3, max_takes=5, duration=16, bonus=("NO_SCRAPS", 0, "No scrapped takes."))
L.set_roles(d, [("Rigger", "Push the long crate into the pit. That's the bridge."),
                ("Grip", "Hit the EXIT switch in the corner - late, so it's still open when the hero gets there."),
                ("Hero", "Wait for the bridge, cross it, through the exit.")])
L.marks([(1, (1200, 0, 0), (4, 6), (-500, -650, 400), "Over the edge"),
         (2, (300, -500, 0), (7, 9.5), (700, -300, 350), "Late switch"),
         (3, (2500, 0, 0), (11, 13.5), (700, 0, 260), "Across the rig")])
L.save_map(MAP)
L.log("Saved " + MAP)
