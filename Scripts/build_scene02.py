"""
Scene02_Heist graybox (3 takes). Run headless:
  UnrealEditor-Cmd.exe EpicAiJam.uproject -run=pythonscript -script="<path>/build_scene02.py" -AllowCommandletRendering

Layout (cm, X forward) - D3 redesign, 12 s takes, par 3:
  Start (0,0). Vault room x 2400..3200, y -500..500; its door (x = 2400) is a gate that is open only
  while LOCK A, B and C (north wall, x 1000/1300/1600, 6 s each) are all on.
  A guard patrols x 900..1700 at y = 600, facing the start.
  Intended: DECOY gets spotted and runs the south wall; INSIDE MAN hits the three locks along the north
  wall; HERO runs for the vault inside the locks' window and grabs the bag.
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
bag = L.prop("MoneyBag", (3000, 0, 0), 70, 20, label="MoneyBag")
if not bag:
    bag = L.box("MoneyBag", (3000, 0, 40), (80, 80, 80), folder="Vault", mesh=L.SPHERE)
    L.no_collision(bag)
L.spawn(unreal.GoalZone, (3000, 0, 100), label="GoalZone_MoneyBag")

# D3: the vault door is a gate that opens only while all three LOCK switches are on (6 s each):
# the INSIDE MAN has to hit them in quick succession, the HERO has to arrive inside that window.
vault_gate = L.spawn(unreal.Gate, (2400, 0, 0), rot=(0, 0, 0), label="VaultGate", folder="Vault")
vault_gate.set_actor_scale3d(unreal.Vector(1, 340.0 / 300.0, 0.92))
locks = []
for i, x in enumerate((1000, 1300, 1600)):
    lock = L.spawn(unreal.TimedSwitch, (x, 1300, 55), label="Lock%d" % (i + 1), folder="Vault")
    lock.set_editor_property("display_name", "LOCK " + "ABC"[i])
    lock.set_editor_property("active_seconds", 6.0)
    locks.append(lock)
vault_gate.set_editor_property("linked_activators", locks)
vault_gate.set_editor_property("require_all_activators", True)

# Guard patrols in front of the start area, looking back at it: the DECOY gets spotted at once.
guard = L.spawn(unreal.StuntGuard, (1700, 600, 100), rot=(0, 180, 0), label="Guard")
guard.set_editor_property("patrol_point_b", unreal.Vector(800, 0, 0))  # local +X = world -X (yaw 180): walks toward the start first
guard.set_editor_property("lose_sight_multiplier", 3.0)

# Some cover crates (static) for flavour, off the running lines.
cover1 = L.box("CoverCrate1", (1900, -900, 60), (120, 120, 120))
cover2 = L.box("CoverCrate2", (-500, 1350, 60), (120, 120, 120))
start = L.spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

L.studio_lights(spots=[((900, -1500, 1300), (1400, 600, 0)), ((3000, 1400, 1300), (2800, 0, 0))])
L.cameras([
    ((-600, -1400, 1400), (1500, 200, 0), 18),   # wide establishing
    ((2000, 1500, 400), (1400, 900, 100), 35),   # lever + guard
    ((2150, -600, 250), (2700, 0, 120), 30),     # vault door / lasers
    ((300, 300, 250), (1100, 700, 100), 40),     # guard close-up
])

# Art pass: Tripo props (keep the south side free - the lure runs there).
L.overlay_static(cover1, "StageCrate")
L.overlay_static(cover2, "StageCrate")
L.film_crew((-550, 900, 0), -40, (-650, 1250, 0), -60,
            lights=[((3400, 1450, 0), -140), ((-700, 400, 0), 0)],
            extra=[("Scaffolding", (2500, 1450, 0), 600, 0), ("PlywoodFlat", (900, 1500, 0), 260, 180),
                   ("StageCrate", (3350, 700, 0), 100, 10), ("StageCrate", (3350, -700, 0), 100, 35)])

d = L.director(start, "Scene 2: The Heist",
           "New trick: ROLES. Three stunt doubles, three jobs, one window. The vault only opens while all three locks are on.",
           par=3, max_takes=5, duration=12,
           bonus=("NOBODY_CAUGHT", 0, "Nobody gets busted."))
L.set_roles(d, [("Decoy", "Get the guard's attention and keep running - south wall, all the way east."),
                ("Inside Man", "Sneak along the north wall and hit LOCK A, B and C - fast, each lasts 6 s."),
                ("Hero", "Run for the vault while the locks are open and grab the bag.")])
L.marks([(1, (1200, -1260, 0), (8, 11), (0, 700, 500), "Decoy run"),
         (2, (800, 1130, 0), (3.5, 6), (0, -700, 500), "Inside job"),
         (3, (2350, -100, 0), (7, 10), (-600, -600, 450), "The vault")])
L.save_map(MAP)
L.log("Saved " + MAP)
