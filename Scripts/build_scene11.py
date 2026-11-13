"""
Scene11_HeistNight - reel 3, remix of Scene 2 (built from build_scene02.py's layout): night, and the guards are
doubled. A second guard patrols in front of the vault, so it takes two decoys now: each one leads its guard in a
circle in its own corner (a slower pursuer never closes on a circle), the inside man hits the three locks, the
hero walks in. The set lights stutter on every CUT (ALightFlicker).
"""
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
import unreal
import retake_levels as L

MAP = "/Game/Retake/Maps/Scene11_HeistNight"
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

# Night shift: a second guard walking north in front of the vault.
guard2 = L.spawn(unreal.StuntGuard, (2000, -1100, 100), rot=(0, 90, 0), label="Guard2")
guard2.set_editor_property("patrol_point_b", unreal.Vector(800, 0, 0))  # yaw 90: local +X = world +Y
guard2.set_editor_property("lose_sight_multiplier", 3.0)

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

L.mood(sun_rot=(-30, 220, 0), sun_color=(0.45, 0.55, 0.9), sun_intensity=0.35, sky_intensity=0.12, spot_scale=1.2)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(a, (unreal.SpotLight, unreal.PointLight)):
        a.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)  # so the CUT flicker can drive them
L.spawn(unreal.LightFlicker, (0, 0, 300), label="LightFlicker")

d = L.director(start, "Scene 11: The Heist (Night)",
           "Same heist, night shoot, and now there are TWO guards. Each needs its own decoy. The vault still wants all three locks on.",
           par=4, max_takes=6, duration=12,
           bonus=("NOBODY_CAUGHT", 0, "Nobody gets busted."))
L.set_roles(d, [("Decoy", "Get the first guard's attention, then lead it round in a BIG circle in the south-west."),
                ("Second Decoy", "Get the vault guard's attention, then circle it in the south-east."),
                ("Inside Man", "North wall: hit LOCK A, B and C fast."),
                ("Hero", "Through the middle, into the vault while the locks are open.")])
L.marks([(1, (150, -1550, 0), (4, 11), (-600, 900, 500), "First decoy"),
         (2, (2850, -1590, 0), (6, 11.5), (-800, 900, 500), "Second decoy"),
         (3, (2350, -100, 0), (6, 9), (-600, 600, 450), "Night vault")])
# S3 story dressing: empty chairs, coffee, clapper, and the crew board by the west wall.
L.director_chairs((-720, 150, 0), yaw=0)
L.coffee((-700, -120, 0))
L.clapperboard((-620, 330, 0), 0, "RETAKE  -  NOV 14 1977")
L.call_board((-790, 650, 0), 0, ["D. KOWALSKI - DIRECTOR", "M. REYES - CAMERA", "T. OKAFOR - SOUND", "L. BAUER - GRIP", "S. PATEL - SAFETY", "J. WARD - STUNTS"], "J. WARD - STUNTS")
L.save_map(MAP)
L.log("Saved " + MAP)
