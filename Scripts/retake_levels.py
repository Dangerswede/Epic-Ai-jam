"""
Shared helpers for the level build scripts (graybox geometry, lights, cameras, director).
Imported by build_scene0X.py; not run on its own.
"""
import math
import unreal

eal = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
CAM_TAG = "DirectorsCutCam"


def log(msg):
    unreal.log("[retake_levels] " + msg)


def new_map(path):
    """Create the map, or open the existing one and clear all its actors (deleting a loaded map fails)."""
    if eal.does_asset_exist(path):
        if not les.load_level(path):
            raise RuntimeError("load_level failed: " + path)
        doomed = [a for a in actors.get_all_level_actors()
                  if not isinstance(a, (unreal.WorldSettings, unreal.Brush))]
        actors.destroy_actors(doomed)
        log("Cleared %d actors in %s" % (len(doomed), path))
        return
    if not les.new_level(path):
        raise RuntimeError("new_level failed: " + path)


def save_map(path=None):
    """Save the current level. save_current_level can silently skip a re-opened map, so fall back to save_map."""
    if les.save_current_level() or not path:
        return
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, path):
        raise RuntimeError("save failed: " + path)


def spawn(cls, loc, rot=(0, 0, 0), label=None, folder=None):
    """rot = (pitch, yaw, roll)."""
    a = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]))
    if label:
        a.set_actor_label(label)
    if folder:
        a.set_folder_path(folder)
    return a


def box(label, center, size, rot=(0, 0, 0), folder="Set", mesh=None):
    """Static mesh box with full size in cm (engine cube = 100 cm)."""
    a = spawn(unreal.StaticMeshActor, center, rot, label, folder)
    a.static_mesh_component.set_static_mesh(mesh or CUBE)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return a


def box_span(label, x0, x1, y0, y1, z0, z1, folder="Set"):
    """Box from min/max corners."""
    return box(label, ((x0 + x1) / 2.0, (y0 + y1) / 2.0, (z0 + z1) / 2.0),
               (abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)), folder=folder)


def no_collision(actor):
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


def ignore_pawns(actor):
    actor.static_mesh_component.set_collision_response_to_channel(
        unreal.CollisionChannel.ECC_PAWN, unreal.CollisionResponseType.ECR_IGNORE)


def tag(actor, *tags):
    actor.set_editor_property("tags", [unreal.Name(t) for t in tags])


def look_at(pos, target):
    return unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*pos), unreal.Vector(*target))


def studio_lights(sun_rot=(-50, 30, 0), spots=()):
    """One directional light + spotlights [(pos, target)] + a weak sky light."""
    sun = spawn(unreal.DirectionalLight, (0, 0, 1000), rot=sun_rot, label="Sun", folder="Lights")
    sun.light_component.set_intensity(3.0)
    for i, (pos, target) in enumerate(spots):
        spot = spawn(unreal.SpotLight, pos, label="StudioLight%d" % (i + 1), folder="Lights")
        spot.set_actor_rotation(look_at(pos, target), False)
        spot.spot_light_component.set_intensity(80000.0)
        spot.spot_light_component.set_editor_property("outer_cone_angle", 35.0)
        spot.spot_light_component.set_editor_property("attenuation_radius", 5000.0)
    sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight", folder="Lights")
    sky.light_component.set_intensity(0.5)


def cameras(shots, wide_index=0):
    """Place Director's Cut cine cameras: shots = [(pos, target, focal_mm)]. The wide shot gets tag WideCam too."""
    out = []
    for i, (pos, target, focal) in enumerate(shots):
        cam = spawn(unreal.CineCameraActor, pos, label="DC_Cam%d" % (i + 1), folder="Cameras")
        cam.set_actor_rotation(look_at(pos, target), False)
        comp = cam.get_cine_camera_component()
        fs = comp.get_editor_property("focus_settings")
        fs.set_editor_property("focus_method", unreal.CameraFocusMethod.DISABLE)
        comp.set_editor_property("focus_settings", fs)
        comp.set_editor_property("current_focal_length", float(focal))
        tag(cam, CAM_TAG, "WideCam") if i == wide_index else tag(cam, CAM_TAG)
        out.append(cam)
    return out


def ghost_class():
    path = "/Game/Retake/Blueprints/BP_TakeGhost"
    if eal.does_asset_exist(path):
        return unreal.BlueprintEditorLibrary.generated_class(unreal.load_asset(path))
    return None


def set_bonus(d, bonus):
    """bonus = (kind, param, text); kind is a unreal.RetakeBonus name, e.g. 'NO_SCRAPS'."""
    if not bonus:
        return
    kind, param, text = bonus
    d.set_editor_property("bonus_type", getattr(unreal.RetakeBonus, kind))
    d.set_editor_property("bonus_param", float(param))
    d.set_editor_property("bonus_text", text)


def director(start, title, shot_list, par, max_takes, duration, bonus=None):
    d = spawn(unreal.RetakeSceneDirector, (0, 0, 200), label="SceneDirector")
    d.set_editor_property("scene_title", title)
    d.set_editor_property("shot_list_text", shot_list)
    d.set_editor_property("player_start", start)
    d.set_editor_property("par_takes", par)
    d.set_editor_property("max_takes", max_takes)
    d.set_editor_property("take_duration_seconds", float(duration))
    gc = ghost_class()
    if gc:
        d.set_editor_property("ghost_class", gc)
    set_bonus(d, bonus)
    return d
