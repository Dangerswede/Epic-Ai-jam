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


def set_roles(d, roles):
    """roles = [(name, task), ...] for take 1, 2, ..."""
    out = []
    for name, task in roles:
        r = unreal.TakeRole()
        r.set_editor_property("name", name)
        r.set_editor_property("task", task)
        out.append(r)
    d.set_editor_property("roles", out)


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


# ---------------------------------------------------------------------------- art pass (Tripo props)
ART_ROOT = "/Game/Retake/Art/Meshes"


def art(name):
    """Loads an imported prop mesh, e.g. art('StageCrate'); None if it isn't imported."""
    path = "%s/SM_%s/StaticMeshes/SM_%s" % (ART_ROOT, name, name)
    return unreal.load_asset(path) if eal.does_asset_exist(path) else None


def prop(name, loc, height, yaw=0.0, label=None):
    """Decorative prop (no collision), uniformly scaled so it is `height` cm tall. Keep props off play paths."""
    mesh = art(name)
    if not mesh:
        log("prop %s not imported - skipped" % name)
        return None
    a = spawn(unreal.StaticMeshActor, loc, (0, yaw, 0), label or ("Prop_" + name), "Dressing")
    comp = a.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    b = mesh.get_bounding_box()
    s = height / max(1.0, b.max.z - b.min.z)
    a.set_actor_scale3d(unreal.Vector(s, s, s))
    return a


def set_art(actor, name):
    """Gameplay actor (crate, plate, lever, explosive): show the prop, keep the graybox as invisible collision."""
    mesh = art(name)
    if mesh and actor:
        actor.set_editor_property("art_mesh_asset", mesh)


def overlay_static(actor, name):
    """Static graybox box (e.g. cover crates): put a prop of the same size on it and hide the box (it still collides)."""
    mesh = art(name)
    if not (mesh and actor):
        return
    b = actor.get_actor_bounds(False)
    origin, extent = b[0], b[1]
    p = spawn(unreal.StaticMeshActor, (origin.x, origin.y, origin.z - extent.z), (0, 0, 0), actor.get_actor_label() + "_Art", "Dressing")
    comp = p.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    mb = mesh.get_bounding_box()
    size = mb.max - mb.min
    p.set_actor_scale3d(unreal.Vector(2 * extent.x / max(1.0, size.x), 2 * extent.y / max(1.0, size.y), 2 * extent.z / max(1.0, size.z)))
    actor.set_actor_hidden_in_game(True)
    actor.static_mesh_component.set_visibility(False)


def film_crew(cam_loc, cam_yaw, chair_loc, chair_yaw, lights=(), extra=()):
    """The usual set dressing: camera on tripod, director's chair with clapperboard, studio lights, extras."""
    prop("CinemaCamera", cam_loc, 170, cam_yaw)
    prop("DirectorChair", chair_loc, 100, chair_yaw)
    prop("Clapperboard", (chair_loc[0] + 70, chair_loc[1], chair_loc[2]), 35, chair_yaw + 30)
    prop("FilmReels", (chair_loc[0] - 80, chair_loc[1] + 40, chair_loc[2]), 45, chair_yaw)
    for loc, yaw in lights:
        prop("StudioSpotlight", loc, 230, yaw)
    for name, loc, height, yaw in extra:
        prop(name, loc, height, yaw)

def marks(items):
    """F1 camera marks: (number, (x, y, floor z), (window start, end), (camera dx, dy, dz), shot name)."""
    for n, loc, win, cam, name in items:
        m = spawn(unreal.CameraMark, loc, label="CameraMark%d" % n, folder="Marks")
        m.set_editor_property("mark_number", n)
        m.set_editor_property("window_start", float(win[0]))
        m.set_editor_property("window_end", float(win[1]))
        m.set_editor_property("camera_location", unreal.Vector(*[float(v) for v in cam]))
        m.set_editor_property("shot_name", name)


# ---------------------------------------------------------------------------- ST2/S3: story scenes
def mood(sun_rot=(-12, 200, 0), sun_color=(1.0, 0.55, 0.3), sun_intensity=2.0, sky_intensity=0.25, spot_scale=0.6):
    """Re-light a level after studio_lights(): low warm sun (dusk), dimmer sky and spots."""
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if isinstance(a, unreal.DirectionalLight):
            a.set_actor_rotation(unreal.Rotator(roll=sun_rot[2], pitch=sun_rot[0], yaw=sun_rot[1]), False)
            a.light_component.set_intensity(sun_intensity)
            a.light_component.set_light_color(unreal.LinearColor(*sun_color, 1.0))
        elif isinstance(a, unreal.SkyLight):
            a.light_component.set_intensity(sky_intensity)
        elif isinstance(a, unreal.SpotLight):
            comp = a.spot_light_component
            comp.set_intensity(comp.intensity * spot_scale)


def dressing(name, loc, size, yaw=0.0, color=(0.35, 0.3, 0.25)):
    """S3 set dressing, never in the player's path: the Tripo prop SM_<name> if it is imported (height = size[2]),
    otherwise a graybox placeholder of `size` cm. No collision either way."""
    a = prop(name, loc, size[2], yaw, label="Dress_" + name)
    if a:
        return a
    b = box("Dress_" + name, (loc[0], loc[1], loc[2] + size[2] / 2.0), size, rot=(0, yaw, 0), folder="Dressing")
    b.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    b.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)
    return b


def sign(text, loc, yaw=0.0, size=40.0, color=(0.95, 0.9, 0.75), label=None):
    """Readable text in the world (posters, tags, chalk boards). Faces +X rotated by yaw."""
    t = spawn(unreal.TextRenderActor, loc, (0, yaw, 0), label or ("Sign_" + text[:16]), "Dressing")
    comp = t.text_render
    comp.set_text(text)
    comp.set_world_size(size)
    comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    comp.set_text_render_color(unreal.Color(int(color[2] * 255), int(color[1] * 255), int(color[0] * 255), 255))
    return t


# S3 story props. Tripo names (ArtSource/SM_<Name>.glb via import_art.py) swap in automatically when imported.
def director_chairs(loc, yaw=0.0, n=2, gap=90):
    """Empty director's chairs side by side."""
    import math
    dx, dy = math.cos(math.radians(yaw + 90)) * gap, math.sin(math.radians(yaw + 90)) * gap
    for i in range(n):
        dressing("DirectorChair", (loc[0] + dx * i, loc[1] + dy * i, loc[2]), (55, 55, 105), yaw)


def coffee(loc, yaw=0.0):
    """A small table with a coffee cup on it (nobody drinking)."""
    dressing("SideTable", loc, (60, 60, 75), yaw)
    dressing("CoffeeCup", (loc[0], loc[1], loc[2] + 75), (9, 9, 12), yaw)


def clapperboard(loc, yaw, text):
    """Clapperboard (placeholder: a slate) with chalk text facing yaw."""
    dressing("Clapperboard", loc, (6, 32, 28), yaw)
    import math
    off = (math.cos(math.radians(yaw)) * 4, math.sin(math.radians(yaw)) * 4)
    sign(text, (loc[0] + off[0], loc[1] + off[1], loc[2] + 12), yaw, 3.2, (0.95, 0.95, 0.9), "Sign_Clapper")


def call_board(loc, yaw, names, keep):
    """Crew board: every name crossed out (red bar) except `keep`."""
    import math
    dressing("CallBoard", loc, (8, 180, 150), yaw)
    fx, fy = math.cos(math.radians(yaw)) * 6, math.sin(math.radians(yaw)) * 6
    sign("CREW - NOV 14, 1977", (loc[0] + fx, loc[1] + fy, loc[2] + 135), yaw, 12, (0.6, 0.08, 0.05), "Sign_CrewHeader")
    lamp = spawn(unreal.PointLight, (loc[0] + fx * 20, loc[1] + fy * 20, loc[2] + 170), label="CrewBoardLamp", folder="Lights")
    lamp.point_light_component.set_intensity(250.0)
    lamp.point_light_component.set_attenuation_radius(450.0)
    lamp.point_light_component.set_light_color(unreal.LinearColor(1.0, 0.85, 0.6, 1.0))
    for i, name in enumerate(names):
        z = loc[2] + 112 - i * 15
        sign(name, (loc[0] + fx, loc[1] + fy, z), yaw, 11, (0.06, 0.06, 0.08), "Sign_Crew_" + name)
        if name != keep:
            sx, sy = -math.sin(math.radians(yaw)), math.cos(math.radians(yaw))
            bar = box("CrossOut", (loc[0] + fx * 1.3, loc[1] + fy * 1.3, z), (2, 3, 2), rot=(0, yaw, 0), folder="Dressing")
            bar.set_actor_scale3d(unreal.Vector(0.02, len(name) * 0.075, 0.02))
            no_collision(bar)


def wreath(loc, yaw, text="FOR JACK"):
    """Funeral wreath on a stand; the ribbon text is small - readable only up close."""
    import math
    if not prop("Wreath", loc, 120, yaw, label="Dress_Wreath"):
        # Placeholder: an upright ring-ish disc on a thin stand.
        cyl = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
        disc = box("Dress_Wreath", (loc[0], loc[1], loc[2] + 85), (70, 70, 12), rot=(90, yaw, 0), folder="Dressing", mesh=cyl)
        no_collision(disc)
        stand = box("Dress_WreathStand", (loc[0], loc[1], loc[2] + 25), (4, 4, 50), folder="Dressing")
        no_collision(stand)
    fx, fy = math.cos(math.radians(yaw)) * 14, math.sin(math.radians(yaw)) * 14
    sign(text, (loc[0] + fx, loc[1] + fy, loc[2] + 70), yaw, 6, (0.45, 0.04, 0.04), "Sign_Wreath")


def newspaper(loc, yaw, headline="STUNTMAN INJURED ON SET"):
    """A newspaper propped up; small headline, readable up close."""
    dressing("Newspaper", loc, (3, 38, 28), yaw)
    import math
    fx, fy = math.cos(math.radians(yaw)) * 3, math.sin(math.radians(yaw)) * 3
    sign(headline, (loc[0] + fx, loc[1] + fy, loc[2] + 22), yaw, 2.6, (0.08, 0.08, 0.08), "Sign_Headline")
    sign("NOV 15, 1977", (loc[0] + fx, loc[1] + fy, loc[2] + 26), yaw, 1.6, (0.08, 0.08, 0.08), "Sign_PaperDate")
