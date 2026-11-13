"""
Scene07_Vault - reel 2. Old pieces: levers + pressure plates + gates (require all). 3 takes, ~12 s each.
A lever in the lobby opens the antechamber gate; the vault door needs BOTH antechamber plates held AND the
vault lever pulled. A take that ends on a plate keeps holding it, so the ORDER is the trick:
  take 1 pulls the lobby lever and takes plate 1, take 2 (through the open gate) takes plate 2,
  take 3 pulls the vault lever and walks in. Every clock in the building is stopped at 4:47.

Layout (cm): lobby x -600..800, y -800..800. Gate 1 at x = 800 (door y -170..170).
Antechamber x 800..2000: plates at (1200, 600) and (1200, -600), vault lever at (1880, 740).
Vault door at x = 2000, vault x 2000..2700, goal at (2450, 0).
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene07_Vault"
L.new_map(MAP)
W = 170
H = 330

L.box_span("Floor", -600, 2700, -800, 800, -20, 0)
L.box_span("Wall_N", -600, 2700, 800, 830, 0, H)
L.box_span("Wall_S", -600, 2700, -830, -800, 0, H)
L.box_span("Wall_W", -630, -600, -830, 830, 0, H)
L.box_span("Wall_E", 2700, 2730, -830, 830, 0, H)
for x, name in ((800, "Lobby"), (2000, "Vault")):
    L.box_span(name + "Wall_N", x - 15, x + 15, W, 800, 0, H)
    L.box_span(name + "Wall_S", x - 15, x + 15, -800, -W, 0, H)
    L.box_span(name + "Lintel", x - 15, x + 15, -W, W, 260, H)

lever1 = L.spawn(unreal.Lever, (300, 740, 15), label="LobbyLever")
gate1 = L.spawn(unreal.Gate, (800, 0, 0), label="AnteGate")
gate1.set_actor_scale3d(unreal.Vector(1, 2 * W / 300.0, 260 / 300.0))
gate1.set_editor_property("linked_activators", [lever1])

plate1 = L.spawn(unreal.PressurePlate, (1200, 600, 40), label="Plate1")
plate2 = L.spawn(unreal.PressurePlate, (1200, -600, 40), label="Plate2")
lever2 = L.spawn(unreal.Lever, (1880, 740, 15), label="VaultLever")
vault = L.spawn(unreal.Gate, (2000, 0, 0), label="VaultDoor")
vault.set_actor_scale3d(unreal.Vector(1, 2 * W / 300.0, 260 / 300.0))
vault.set_editor_property("linked_activators", [plate1, plate2, lever2])
vault.set_editor_property("require_all_activators", True)

L.spawn(unreal.GoalZone, (2450, 0, 100), label="GoalZone_Vault")
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(sun_rot=(-70, 30, 0), spots=[((200, -500, 1200), (300, 300, 0)), ((1400, 0, 1200), (1300, 0, 0)), ((2400, 400, 1100), (2400, 0, 0))])
L.mood(sun_rot=(-70, 30, 0), sun_color=(0.95, 0.9, 0.8), sun_intensity=1.0, sky_intensity=0.3, spot_scale=0.8)

# S3: every clock stopped at 4:47.
L.wall_clock((-585, 0, 230), 0)
L.wall_clock((1000, 785, 230), -90)
L.wall_clock((2685, 0, 230), 180)
L.set_art(lever1, "WallLever")
L.set_art(lever2, "WallLever")
L.prop("MoneyBag", (2550, 300, 0), 60, 30)

L.cameras([((-500, -700, 800), (800, 0, 0), 20), ((1900, -700, 700), (1300, 0, 0), 24), ((2650, -500, 280), (2200, 0, 80), 30)])
d = L.director(start, "Scene 7: The Vault",
               "The vault wants both plates held AND its lever pulled - and the antechamber only opens from the lobby. "
               "Plan the order. The clocks have stopped.",
               par=3, max_takes=5, duration=12, bonus=("NO_SCRAPS", 0, "No scrapped takes."))
L.set_roles(d, [("Inside Man", "Pull the lobby lever, go through, and stand on plate 1 (north)."),
                ("Lookout", "Through the gate, stand on plate 2 (south)."),
                ("Safecracker", "Pull the vault lever and walk in.")])
L.marks([(1, (300, 620, 0), (0.5, 2.5), (500, -400, 300), "The lever"),
         (2, (1200, -600, 0), (2.5, 6), (-300, 500, 400), "Plate two"),
         (3, (2300, 0, 0), (5, 9), (300, -600, 260), "Inside the vault")])
L.save_map(MAP)
L.log("Saved " + MAP)
