"""
Surface textures for the graybox levels.

1. Imports ArtSource/Surfaces/T_Surf<Name>_BaseColor/_Normal/_ORM.png -> /Game/Retake/Art/Surfaces
   (made by Scripts/make_surface_textures.py).
2. (Re)builds M_RetakeSurface: world-aligned (triplanar) mapping, so the stretched engine cubes the levels
   are built from get evenly sized texture tiles without UVs. Params: BaseColor/Normal/ORM (texture objects),
   TileSize (R = cm per tile horizontally, G = vertically), BaseColorTint, RoughnessScale, MetallicScale.
3. One material instance per surface look (MI_Surf_*).
4. Opens every Retake map and puts a surface on each graybox box (StaticMeshActor in the "Set" folder
   with an engine BasicShapes mesh) by its label, using the per-map RULES below. Saves the maps.

Safe to re-run (also after rebuilding a scene with build_sceneXX.py). Run headless with the editor closed:
  UnrealEditor-Cmd.exe EpicAiJam.uproject -ExecutePythonScript="<path>/setup_surfaces.py" -unattended
"""
import os
import re
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals()
                                    else r"D:\Projekt\EpicAiJam\Scripts", ".."))
SRC = os.path.join(ROOT, "ArtSource", "Surfaces")
DIR = "/Game/Retake/Art/Surfaces"
MASTER = DIR + "/M_RetakeSurface"
MAPS = "/Game/Retake/Maps/"

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# look: (texture set, tint, (tile cm horizontal, vertical), roughness scale)
SURFACES = {
    "Concrete":    ("Concrete", (1.0, 1.0, 1.0), (350, 350), 1.0),
    "PitConcrete": ("Concrete", (0.75, 0.75, 0.78), (300, 300), 1.0),
    "Plywood":     ("Plywood", (0.9, 0.85, 0.8), (240, 240), 1.0),
    "Brick":       ("Brick", (1.0, 1.0, 1.0), (170, 170), 1.0),
    "Wood":        ("Wood", (1.0, 1.0, 1.0), (240, 240), 1.0),
    "WoodWall":    ("Wood", (0.7, 0.62, 0.55), (300, 300), 1.0),
    "Metal":       ("Metal", (1.0, 1.0, 1.0), (200, 200), 1.0),
    "Asphalt":     ("Asphalt", (1.0, 1.0, 1.0), (400, 400), 1.0),
    "Painted":     ("Painted", (0.85, 0.83, 0.8), (300, 300), 1.0),
    "PaintedDark": ("Painted", (0.18, 0.18, 0.21), (300, 300), 1.0),
    "PaintedRed":  ("Painted", (0.35, 0.08, 0.07), (300, 300), 1.0),
}

# Per map: ordered (label regex, look); first match wins. None = leave as it is.
STUDIO_BRIDGE = [(r"Pillar", "Metal"), (r"Bridge|Ramp", "Wood"), (r"Floor", "Concrete")]
HEIST = [(r"Vault", "Metal"), (r"Crate", "Wood"), (r"Floor", "Concrete")]
CHASE = [(r"Breakable", "Plywood"), (r"Wall", "Brick"), (r"Floor", "Asphalt")]
GAP = [(r"StuntMat", None), (r"Pit", "PitConcrete"), (r"Floor", "Concrete"), (r"Wall", "Plywood")]
RULES = {
    "Scene01_Bridge": STUDIO_BRIDGE,
    "Scene08_BridgeReshoot": STUDIO_BRIDGE,
    "Scene02_Heist": HEIST,
    "Scene11_HeistNight": HEIST,
    "Scene03_Chase": CHASE,
    "Scene14_ChaseFinal": CHASE,
    "Scene04_Finale": [(r"PitRail|WaterTower|LaserDoor", "Metal"), (r"Billboard", "Plywood"), (r"Crate", "Wood"),
                       (r"Wall|Building|Parapet", "Brick"), (r"Floor|RoofRamp", "Asphalt")],
    "Scene05_TheGap": GAP,
    "Scene15_Rehearsal": GAP,
    "Scene05_Rooftop": [(r"Roof", "Asphalt"), (r"Wall|Lintel|Parapet", "Brick")],
    "Scene06_Saloon": [(r"Wall", "WoodWall"), (r"Floor|Balcony|Bar", "Wood")],
    "Scene07_Vault": [(r"Wall|Lintel", "Metal"), (r"Floor", "Concrete")],
    "Scene09_Backlot": [(r"Lot", "Asphalt"), (r"Facade", "Brick"), (r"Fence", "Plywood"), (r"Boarded", "Wood")],
    "Scene10_EmptyStage": [(r"StageFloor", "Wood"), (r"Wall", "PaintedDark")],
    "Scene12_TheRig": [(r"PitRail", "Metal"), (r"Floor", "Concrete"), (r"Room|Corr", "Painted")],
    "Scene13_ProjectionRoom": [(r"Floor", "Concrete"), (r"Wall|Lintel", "PaintedRed")],
}


def log(msg):
    print("[setup_surfaces] " + msg)
    unreal.log("[setup_surfaces] " + msg)


# ---------------------------------------------------------------------------- textures
def import_texture(fname):
    name = os.path.splitext(fname)[0]
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SRC, fname))
    task.set_editor_property("destination_path", DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    tex = unreal.load_asset(DIR + "/" + name)
    if not tex:
        log("FAILED texture " + fname)
        return None
    if name.endswith("_Normal"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif name.endswith("_ORM"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    tex.set_editor_property("max_texture_size", 1024)
    eal.save_loaded_asset(tex)
    return tex


def texture(set_name, kind):
    return unreal.load_asset("%s/T_Surf%s_%s" % (DIR, set_name, kind))


# ---------------------------------------------------------------------------- master material
TRI = """
float3 w = pow(abs(N), 4.0);
w /= (w.x + w.y + w.z);
float2 uvX = float2(P.y, -P.z) / S;
float2 uvY = float2(P.x, -P.z) / S;
float2 uvZ = float2(P.x, P.y) / S;
"""
CODE_COLOR = TRI + """
float3 cx = Texture2DSample(Tex, TexSampler, uvX).rgb;
float3 cy = Texture2DSample(Tex, TexSampler, uvY).rgb;
float3 cz = Texture2DSample(Tex, TexSampler, uvZ).rgb;
return cx * w.x + cy * w.y + cz * w.z;
"""
# DirectX tangent normals, re-oriented per projection into world space (UDN blend).
CODE_NORMAL = TRI + """
float2 tx = Texture2DSample(Tex, TexSampler, uvX).rg * 2.0 - 1.0;
float2 ty = Texture2DSample(Tex, TexSampler, uvY).rg * 2.0 - 1.0;
float2 tz = Texture2DSample(Tex, TexSampler, uvZ).rg * 2.0 - 1.0;
float3 n = N + w.x * float3(0.0, tx.x, tx.y) + w.y * float3(ty.x, 0.0, ty.y) + w.z * float3(tz.x, -tz.y, 0.0);
return normalize(n);
"""


def make_master():
    if eal.does_asset_exist(MASTER):
        mat = unreal.load_asset(MASTER)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset("M_RetakeSurface", DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("tangent_space_normal", False)

    def expr(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def tex_obj(name, default, sampler, y):
        e = expr(unreal.MaterialExpressionTextureObjectParameter, -900, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("sampler_type", sampler)
        e.set_editor_property("texture", default)
        return e

    def scalar(name, value, y):
        e = expr(unreal.MaterialExpressionScalarParameter, -500, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", value)
        return e

    def mask(src, out, x, y, r=False, g=False, b=False):
        m = expr(unreal.MaterialExpressionComponentMask, x, y)
        m.set_editor_property("r", r)
        m.set_editor_property("g", g)
        m.set_editor_property("b", b)
        m.set_editor_property("a", False)
        mel.connect_material_expressions(src, out, m, "")
        return m

    def mul(a, b, x, y):
        m = expr(unreal.MaterialExpressionMultiply, x, y)
        mel.connect_material_expressions(a, "", m, "A")
        mel.connect_material_expressions(b, "", m, "B")
        return m

    pos = expr(unreal.MaterialExpressionWorldPosition, -1300, -100)
    nrm = expr(unreal.MaterialExpressionVertexNormalWS, -1300, 0)
    tile = expr(unreal.MaterialExpressionVectorParameter, -1300, 100)
    tile.set_editor_property("parameter_name", "TileSize")
    tile.set_editor_property("default_value", unreal.LinearColor(300, 300, 0, 0))
    tile_rg = mask(tile, "", -1100, 100, r=True, g=True)

    base = tex_obj("BaseColor", texture("Concrete", "BaseColor"), unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -400)
    norm = tex_obj("Normal", texture("Concrete", "Normal"), unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 0)
    orm = tex_obj("ORM", texture("Concrete", "ORM"), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 400)

    def tri(code, tex_e, y):
        c = expr(unreal.MaterialExpressionCustom, -600, y)
        c.set_editor_property("code", code)
        c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        c.set_editor_property("description", "Triplanar")
        ins = []
        for n in ("Tex", "P", "N", "S"):
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        mel.connect_material_expressions(tex_e, "", c, "Tex")
        mel.connect_material_expressions(pos, "", c, "P")
        mel.connect_material_expressions(nrm, "", c, "N")
        mel.connect_material_expressions(tile_rg, "", c, "S")
        return c

    col = tri(CODE_COLOR, base, -400)
    tint = expr(unreal.MaterialExpressionVectorParameter, -500, -250)
    tint.set_editor_property("parameter_name", "BaseColorTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    tint_rgb = mask(tint, "", -350, -250, r=True, g=True, b=True)
    mel.connect_material_property(mul(col, tint_rgb, -200, -350), "", unreal.MaterialProperty.MP_BASE_COLOR)

    nn = tri(CODE_NORMAL, norm, 0)
    mel.connect_material_property(nn, "", unreal.MaterialProperty.MP_NORMAL)

    o = tri(CODE_COLOR, orm, 400)
    mel.connect_material_property(mask(o, "", -350, 400, r=True), "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(mul(mask(o, "", -350, 500, g=True), scalar("RoughnessScale", 1.0, 600), -200, 520), "",
                                  unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mul(mask(o, "", -350, 700, b=True), scalar("MetallicScale", 1.0, 800), -200, 720), "",
                                  unreal.MaterialProperty.MP_METALLIC)
    try:
        mel.recompile_material(mat)
    except Exception as e:  # noqa - headless: shaders compile when the editor/cooker loads it
        log("  (material not compiled now: %s)" % e)
    eal.save_loaded_asset(mat)
    log("Built " + MASTER)
    return mat


def make_instances(master):
    out = {}
    for look, (tset, tint, tile, rough) in SURFACES.items():
        path = "%s/MI_Surf_%s" % (DIR, look)
        mi = unreal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
            "MI_Surf_" + look, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for kind in ("BaseColor", "Normal", "ORM"):
            t = texture(tset, kind)
            if t:
                mel.set_material_instance_texture_parameter_value(mi, kind, t)
            else:
                log("  missing T_Surf%s_%s" % (tset, kind))
        mel.set_material_instance_vector_parameter_value(mi, "BaseColorTint", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
        mel.set_material_instance_vector_parameter_value(mi, "TileSize", unreal.LinearColor(float(tile[0]), float(tile[1]), 0, 0))
        mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rough)
        eal.save_loaded_asset(mi)
        out[look] = mi
    log("Material instances: " + ", ".join(sorted(out)))
    return out


# ---------------------------------------------------------------------------- levels
def apply_to_maps(mis):
    for map_name, rules in RULES.items():
        path = MAPS + map_name
        if not eal.does_asset_exist(path):
            log("map %s missing - skipped" % map_name)
            continue
        if not les.load_level(path):
            log("load_level failed: " + path)
            continue
        done, unmatched = 0, []
        for a in actors.get_all_level_actors():
            if not isinstance(a, unreal.StaticMeshActor):
                continue
            if str(a.get_folder_path()) != "Set":
                continue
            comp = a.static_mesh_component
            mesh = comp.static_mesh
            if not mesh or not mesh.get_path_name().startswith("/Engine/BasicShapes/"):
                continue
            label = a.get_actor_label()
            look = "?"
            for pattern, lk in rules:
                if re.search(pattern, label):
                    look = lk
                    break
            if look == "?":
                unmatched.append(label)
                continue
            if look is None:
                continue
            comp.set_material(0, mis[look])
            done += 1
        if not les.save_current_level():
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
            unreal.EditorLoadingAndSavingUtils.save_map(world, path)
        log("%s: %d surfaces set%s" % (map_name, done, ("; no rule for: " + ", ".join(unmatched)) if unmatched else ""))


# ---------------------------------------------------------------------------- main
if not eal.does_directory_exist(DIR):
    eal.make_directory(DIR)
files = sorted(f for f in os.listdir(SRC) if f.startswith("T_Surf") and f.endswith(".png")) if os.path.isdir(SRC) else []
log("Importing %d textures from %s" % (len(files), SRC))
for f in files:
    import_texture(f)
master = make_master()
apply_to_maps(make_instances(master))
log("Done")
