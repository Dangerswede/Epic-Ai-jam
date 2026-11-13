"""
Builds the graybox level /Game/Retake/Maps/Scene01_Bridge plus M_Ghost and BP_TakeGhost.
Run headless:
  UnrealEditor-Cmd.exe EpicAiJam.uproject -run=pythonscript -script="<path>/build_scene01.py"
Safe to re-run: overwrites the map and recreates the assets.

Layout (cm, X = forward):
  Player start (0,0) on the studio floor.
  Ramp x 200..1000 up to the bridge (z = 300).
  Bridge x 1000..2800, 300 wide. Gate across it at x = 1800.
  Goal zone on the far end of the bridge (x = 2600).
  Pressure plate on the floor at (2400, 900): far from the ramp, so one take
  can't press it and then get through the gate before it closes -> needs 2 takes.
"""
import unreal

MAP_PATH = "/Game/Retake/Maps/Scene01_Bridge"
MAT_DIR = "/Game/Retake/Materials"
BP_DIR = "/Game/Retake/Blueprints"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("[build_scene01] " + msg)


def recreate(path):
    if eal.does_asset_exist(path):
        eal.delete_asset(path)


# ---------------------------------------------------------------- ghost material
def make_ghost_material():
    path = MAT_DIR + "/M_Ghost"
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)  # referenced by BP_TakeGhost; keep it
    mat = asset_tools.create_asset("M_Ghost", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)

    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.70, 0.55, 0.35, 1.0))
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    opacity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.45)
    mel.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    log("Created " + path)
    return mat


# ---------------------------------------------------------------- ghost blueprint
def make_ghost_blueprint(ghost_mat):
    path = BP_DIR + "/BP_TakeGhost"
    if eal.does_asset_exist(path):
        return unreal.BlueprintEditorLibrary.generated_class(unreal.load_asset(path))
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.TakeGhost)
    bp = asset_tools.create_asset("BP_TakeGhost", BP_DIR, None, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(bp))
    cdo.set_editor_property("ghost_material", ghost_mat)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp)
    log("Created " + path)
    return unreal.BlueprintEditorLibrary.generated_class(bp)


# ---------------------------------------------------------------- level
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")


def spawn(cls, loc, rot=(0, 0, 0), label=None):
    a = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        a.set_actor_label(label)
    return a


def box(label, center, size, rot=(0, 0, 0)):
    """Static cube with full size in cm (engine cube is 100 cm)."""
    a = spawn(unreal.StaticMeshActor, center, rot, label)
    a.static_mesh_component.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    a.set_folder_path("Set")
    return a


def build_level(ghost_class):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    import os, sys
    sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
    import retake_levels
    retake_levels.new_map(MAP_PATH)

    # Studio floor: 6000 x 4000, top at z = 0.
    box("StudioFloor", (1500, 0, -10), (6000, 4000, 20))

    # Ramp from (200, z0) to (1000, z300).
    import math
    run, rise = 800.0, 300.0
    length = math.hypot(run, rise)
    pitch = math.degrees(math.atan2(rise, run))
    box("Ramp", (600, 0, 150 - 10), (length, 300, 20), rot=(pitch, 0, 0))

    # Bridge deck x 1000..2800, top at z = 300, plus support pillars.
    box("Bridge", (1900, 0, 290), (1800, 300, 20))
    for x in (1100, 1900, 2700):
        for y in (-140, 140):
            box("BridgePillar", (x, y, 140), (20, 20, 280))

    # D2: two gates on the bridge, each held open 5 s by its own timed switch on the studio floor.
    # GRIP (take 1) hits switch A by the ramp, sprints under the bridge and hits switch B;
    # HERO (take 2) crosses through both windows.
    gate_a = spawn(unreal.Gate, (1500, 0, 300), label="GateA")
    gate_b = spawn(unreal.Gate, (2300, 0, 300), label="GateB")
    switch_a = spawn(unreal.TimedSwitch, (300, -500, 55), label="SwitchA")
    switch_b = spawn(unreal.TimedSwitch, (2600, -500, 55), label="SwitchB")
    for sw, name in ((switch_a, "SWITCH A"), (switch_b, "SWITCH B")):
        sw.set_editor_property("display_name", name)
        sw.set_editor_property("active_seconds", 5.0)
    gate_a.set_editor_property("linked_activators", [switch_a])
    gate_b.set_editor_property("linked_activators", [switch_b])

    # Goal zone on the far end of the bridge (box extent 100, bottom on the deck).
    spawn(unreal.GoalZone, (2650, 0, 400), label="GoalZone")

    # Safety net far below the floor.
    kill = spawn(unreal.KillVolume, (1500, 0, -600), label="KillVolume")
    kill.set_actor_scale3d(unreal.Vector(10, 10, 1))

    # Player start, facing +X.
    start = spawn(unreal.PlayerStart, (0, 0, 100), label="PlayerStart")

    # Lighting: one directional "sun" + two studio spotlights (+ weak sky light so shadows aren't black).
    sun = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=(-50, 30, 0), label="Sun")
    sun.light_component.set_intensity(3.0)
    for i, (x, y) in enumerate(((800, -1400), (2400, 1400))):
        target = unreal.Vector(1900 if i == 0 else 2200, 0 if i == 0 else 600, 0)
        pos = unreal.Vector(x, y, 1200)
        rot = unreal.MathLibrary.find_look_at_rotation(pos, target)
        spot = spawn(unreal.SpotLight, (x, y, 1200), label="StudioLight%d" % (i + 1))
        spot.set_actor_rotation(rot, False)
        spot.spot_light_component.set_intensity(80000.0)
        spot.spot_light_component.set_editor_property("outer_cone_angle", 35.0)
        spot.spot_light_component.set_editor_property("attenuation_radius", 4000.0)
    sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
    sky.light_component.set_intensity(0.5)

    # Scene director.
    director = spawn(unreal.RetakeSceneDirector, (0, 0, 200), label="SceneDirector")
    director.set_editor_property("scene_title", "Scene 1: The Bridge")
    director.set_editor_property("shot_list_text",
                                 "New trick: TIMED SWITCHES. A switch holds its gate open for 5 seconds. "
                                 "Take 1 opens the way, take 2 walks it. Press R to call CUT - your take replays next time.")
    director.set_editor_property("player_start", start)
    if ghost_class:
        director.set_editor_property("ghost_class", ghost_class)

    # Director's Cut cameras (T10).
    import os
    import sys
    sys.path.append(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else r"D:\Projekt\EpicAiJam\Scripts")
    import retake_levels as L
    director.set_editor_property("par_takes", 2)
    director.set_editor_property("max_takes", 4)
    director.set_editor_property("take_duration_seconds", 15.0)
    L.set_roles(director, [("Grip", "Hit switch A by the ramp, sprint under the bridge, hit switch B."),
                           ("Hero", "Cross the bridge while both gates are open.")])
    FINAL_N = 9  # director's note: playtest final take + ~30%
    L.set_bonus(director, ("FINAL_TAKE_UNDER_SECONDS", FINAL_N, "Do the final take in under %d seconds." % FINAL_N))
    L.cameras([
        ((-700, -1600, 1300), (1500, 0, 150), 18),   # wide
        ((2400, 900, 250), (1800, 0, 350), 30),      # gate from below, near the plate
        ((900, -700, 600), (1800, 0, 300), 35),      # bridge side
        ((3100, 300, 600), (2300, 0, 300), 28),      # goal end looking back
    ])

    # Art pass: Tripo props (outside the play paths).
    L.film_crew((-450, -700, 0), 30, (-600, -1000, 0), 60,
                lights=[((600, -1600, 0), 60), ((3200, 1700, 0), -120), ((-700, 900, 0), -30)],
                extra=[("Scaffolding", (1500, -1700, 0), 600, 0), ("Scaffolding", (3600, -1700, 0), 600, 0),
                       ("PlywoodFlat", (4100, 1300, 0), 260, 180), ("PlywoodFlat", (3900, -1100, 0), 260, 160),
                       ("StageCrate", (-900, -400, 0), 90, 20), ("StageCrate", (-900, -300, 90), 70, 50)])

    L.marks([(1, (1170, -430, 0), (2, 4.5), (-500, -600, 350), "Under the bridge"),
             (2, (1600, 0, 300), (3, 6), (-400, -700, 250), "Bridge run"),
             (3, (2350, -480, 0), (5, 7.5), (300, -600, 300), "Switch B")])
    # S3 story dressing (off the play paths: north-west of the start).
    L.director_chairs((-800, 450, 0), yaw=-20)
    L.coffee((-650, 800, 0))
    L.clapperboard((-560, 330, 0), -30, "RETAKE  -  NOV 14 1977")
    L.save_map(MAP_PATH)
    log("Saved " + MAP_PATH)


mat = make_ghost_material()
ghost_cls = make_ghost_blueprint(mat)
build_level(ghost_cls)
log("DONE")
