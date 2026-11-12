"""
Scene02_Heist graybox (3 takes). Run headless:
  UnrealEditor-Cmd.exe EpicAiJam.uproject -run=pythonscript -script="<path>/build_scene02.py" -AllowCommandletRendering

Layout (cm, X forward):
  Start (0,0). Vault room x 2400..3200, y -500..500, door in the west wall guarded by 3 laser beams.
  Lever at (1400, 1100) disables the lasers. A guard patrols x 1100..1800 at y = 700, watching the lever.
  Intended: take 1 lures the guard away (run to the far corner), take 2 pulls the lever,
  take 3 walks through the dead lasers and grabs the money bag (goal) in the vault.
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene02_Heist"
L.new_map(MAP)

# Studio floor + safety net.
L.box_span("StudioFloor", -800, 3600, -1600, 1600, -20, 0)
kill = L.spawn(unreal.KillVolume, (1400, 0, -600), label="KillVolume")
kill.set_actor_scale3d(unreal.Vector(10, 10, 1))

# Vault: walls 30 thick, 300 tall. Door gap y -170..170 in the west wall.
H = 300
L.box_span("VaultWall_N", 2400, 3200, 485, 515, 0, H, folder="Vault")
L.box_span("VaultWall_S", 2400, 3200, -515, -485, 0, H, folder="Vault")
L.box_span("VaultWall_E", 3185, 3215, -515, 515, 0, H, folder="Vault")
L.box_span("VaultWall_W1", 2385, 2415, 170, 515, 0, H, folder="Vault")
L.box_span("VaultWall_W2", 2385, 2415, -515, -170, 0, H, folder="Vault")
L.box_span("VaultDoorLintel", 2385, 2415, -170, 170, 230, H, folder="Vault")

# Money bag (visual) + goal zone.
bag = L.box("MoneyBag", (3000, 0, 40), (80, 80, 80), folder="Vault", mesh=L.SPHERE)
L.no_collision(bag)
L.spawn(unreal.GoalZone, (3000, 0, 100), label="GoalZone_MoneyBag")

# Lever and lasers across the vault door (beam runs along local +X -> yaw 90 = world +Y).
lever = L.spawn(unreal.Lever, (1400, 1100, 15), label="Lever")
lasers = []
for i, z in enumerate((40, 110, 180)):
    laser = L.spawn(unreal.LaserBeam, (2400, -170, z), rot=(0, 90, 0), label="Laser%d" % (i + 1), folder="Vault")
    laser.set_editor_property("beam_length", 340.0)
    laser.set_editor_property("linked_activators", [lever])
    lasers.append(laser)

# Guard watching the lever.
guard = L.spawn(unreal.StuntGuard, (1100, 700, 100), label="Guard")
guard.set_editor_property("patrol_point_b", unreal.Vector(700, 0, 0))
guard.set_editor_property("lose_sight_multiplier", 3.0)

# Some cover crates (static) for flavour.
cover1 = L.box("CoverCrate1", (600, 450, 60), (120, 120, 120))
cover2 = L.box("CoverCrate2", (-300, -1100, 60), (120, 120, 120))

start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((900, -1500, 1300), (1400, 600, 0)), ((3000, 1400, 1300), (2800, 0, 0))])
L.cameras([
    ((-600, -1400, 1400), (1500, 200, 0), 18),   # wide establishing
    ((2000, 1500, 400), (1400, 900, 100), 35),   # lever + guard
    ((2150, -600, 250), (2700, 0, 120), 30),     # vault door / lasers
    ((300, 300, 250), (1100, 700, 100), 40),     # guard close-up
])

# Art pass: Tripo props (keep the south side free - the lure runs there).
L.set_art(lever, "WallLever")
L.overlay_static(cover1, "StageCrate")
L.overlay_static(cover2, "StageCrate")
L.film_crew((-550, 900, 0), -40, (-650, 1250, 0), -60,
            lights=[((3400, 1450, 0), -140), ((-700, 400, 0), 0)],
            extra=[("Scaffolding", (2500, 1450, 0), 600, 0), ("PlywoodFlat", (900, 1500, 0), 260, 180),
                   ("StageCrate", (3350, 700, 0), 100, 10), ("StageCrate", (3350, -700, 0), 100, 35)])

L.director(start, "Scene 2: The Heist",
           "Three takes. Distract, disable, grab. Make it look easy.",
           par=3, max_takes=5, duration=30,
           bonus=("NOBODY_CAUGHT", 0, "Nobody gets busted."))
L.save_map(MAP)
L.log("Saved " + MAP)
