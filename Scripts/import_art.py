"""
Art import pipeline.

1. Creates (once) /Game/Retake/Art/Materials/M_RetakeMaster: texture params BaseColor, Normal, ORM
   (R = AO, G = Roughness, B = Metallic) plus BaseColorTint, RoughnessScale, MetallicScale.
2. Imports everything in ArtSource/:
     meshes   *.glb *.gltf *.fbx *.obj  -> /Game/Retake/Art/Meshes/SM_<Name>
     textures *.png *.jpg *.jpeg *.tga  -> /Game/Retake/Art/Textures/T_<Name>
   File names may already carry the prefix (SM_StudioSpotlight.glb, T_Plywood_BaseColor.png).
3. For each mesh creates MI_<Name> (parent M_RetakeMaster) and assigns textures found by name:
     T_<Name>_BaseColor / _Normal / _ORM  (also accepts _Albedo/_Diffuse, _N, _ARM).
   The material instance is put on every material slot of the mesh.
4. Scale check: meshes whose largest bound is < 5 cm or > 50 m are assumed to be in the wrong
   unit and rescaled (x100 or x0.01) at import - reported in the log.
5. Appends an ASSETS.md row stub per newly imported file.

Run headless (editor closed). Use -ExecutePythonScript (full editor, needed for the mesh rescale):
  UnrealEditor-Cmd.exe EpicAiJam.uproject -ExecutePythonScript="<path>/import_art.py" -unattended -nullrhi
"""
import os
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals()
                                    else r"D:\Projekt\EpicAiJam\Scripts", ".."))
SRC = os.path.join(ROOT, "ArtSource")
ART = "/Game/Retake/Art"
MESH_DIR, TEX_DIR, MAT_DIR = ART + "/Meshes", ART + "/Textures", ART + "/Materials"
MASTER = MAT_DIR + "/M_RetakeMaster"
MESH_EXT = (".glb", ".gltf", ".fbx", ".obj")
TEX_EXT = (".png", ".jpg", ".jpeg", ".tga")
SUFFIXES = {
    "base_color": ("_BaseColor", "_Albedo", "_Diffuse", "_Color"),
    "normal": ("_Normal", "_N"),
    "orm": ("_ORM", "_ARM"),
}

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(msg):
    print("[import_art] " + msg)
    unreal.log("[import_art] " + msg)


def strip_prefix(name, prefix):
    return name[len(prefix):] if name.startswith(prefix) else name


def default_orm_texture():
    """A 4x4 white linear (non-sRGB, Masks) texture so the ORM sampler type matches imported ORM maps."""
    path = MAT_DIR + "/T_DefaultORM"
    if not eal.does_asset_exist(path):
        import struct, tempfile, zlib
        raw = b"".join(b"\x00" + b"\xff\xff\xff" * 4 for _ in range(4))
        def chunk(t, d):
            return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
        png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) \
            + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
        tmp = os.path.join(tempfile.gettempdir(), "T_DefaultORM.png")
        with open(tmp, "wb") as f:
            f.write(png)
        import_file(tmp, MAT_DIR, "T_DefaultORM")
        tex = unreal.load_asset(path)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
        eal.save_loaded_asset(tex)
    return path


# ---------------------------------------------------------------------------- master material
def make_master():
    if eal.does_asset_exist(MASTER):
        return unreal.load_asset(MASTER)
    mat = tools.create_asset("M_RetakeMaster", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())

    def tex_param(name, default, sampler, x, y):
        e = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("texture", unreal.load_asset(default))
        e.set_editor_property("sampler_type", sampler)
        return e

    def scalar(name, value, x, y):
        e = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", value)
        return e

    def mul(a, a_out, b, b_out, x, y):
        m = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, x, y)
        mel.connect_material_expressions(a, a_out, m, "A")
        mel.connect_material_expressions(b, b_out, m, "B")
        return m

    base = tex_param("BaseColor", "/Engine/EngineResources/WhiteSquareTexture", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -800, -200)
    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, 50)
    tint.set_editor_property("parameter_name", "BaseColorTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mel.connect_material_property(mul(base, "RGB", tint, "", -400, -150), "", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = tex_param("Normal", "/Engine/EngineMaterials/FlatNormal", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -800, 250)
    mel.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    orm = tex_param("ORM", default_orm_texture(), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, -800, 550)
    mel.connect_material_property(orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(mul(orm, "G", scalar("RoughnessScale", 0.8, -800, 800), "", -400, 600), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(mul(orm, "B", scalar("MetallicScale", 0.0, -800, 900), "", -400, 750), "", unreal.MaterialProperty.MP_METALLIC)

    try:
        mel.recompile_material(mat)
    except Exception as e:  # -nullrhi: shaders compile later when the editor/cooker loads it
        log("  (material not compiled now: %s)" % e)
    eal.save_loaded_asset(mat)
    log("Created " + MASTER)
    return mat


# ---------------------------------------------------------------------------- import helpers
def import_file(path, dest_dir, dest_name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", dest_dir)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    # Interchange may append suffixes; find what landed.
    exact = dest_dir + "/" + dest_name
    if eal.does_asset_exist(exact):
        return [unreal.load_asset(exact)]
    return [unreal.load_asset(p) for p in task.get_editor_property("imported_object_paths") or []]


def find_texture(name, kind):
    for suffix in SUFFIXES[kind]:
        path = TEX_DIR + "/T_" + name + suffix
        if eal.does_asset_exist(path):
            return unreal.load_asset(path)
    return None


def normalize_scale(mesh):
    box = mesh.get_bounding_box()
    size = box.max - box.min
    largest = max(size.x, size.y, size.z)
    factor = 1.0
    if 0 < largest < 5.0:
        factor = 100.0     # authored in meters
    elif largest > 5000.0:
        factor = 0.01      # authored in millimeters/oversized
    if factor != 1.0:
        # Static mesh source models: apply via the first LOD build settings.
        try:
            sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            settings = sm.get_lod_build_settings(mesh, 0)
            settings.set_editor_property("build_scale3d", unreal.Vector(factor, factor, factor))
            sm.set_lod_build_settings(mesh, 0, settings)
            log("  rescaled %s x%s (largest bound was %.1f cm)" % (mesh.get_name(), factor, largest))
        except Exception as e:  # noqa
            log("  WARNING %s looks mis-scaled (largest bound %.1f cm), fix by hand: %s" % (mesh.get_name(), largest, e))
    return largest * factor


# ---------------------------------------------------------------------------- main
master = make_master()
new_files = []
if not os.path.isdir(SRC):
    log("No ArtSource folder at " + SRC)
else:
    files = sorted(os.listdir(SRC))
    # Textures first so materials can find them.
    for fname in files:
        base, ext = os.path.splitext(fname)
        if ext.lower() not in TEX_EXT:
            continue
        name = "T_" + strip_prefix(base, "T_")
        existed = eal.does_asset_exist(TEX_DIR + "/" + name)
        assets = import_file(os.path.join(SRC, fname), TEX_DIR, name)
        tex = assets[0] if assets else None
        if not tex:
            log("FAILED texture " + fname)
            continue
        if name.endswith(("_Normal", "_N")):
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif name.endswith(("_ORM", "_ARM")):
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        eal.save_loaded_asset(tex)
        if not existed:
            new_files.append(fname)
        log("Texture " + fname + " -> " + name)

    for fname in files:
        base, ext = os.path.splitext(fname)
        if ext.lower() not in MESH_EXT:
            continue
        short = strip_prefix(base, "SM_")
        name = "SM_" + short
        existed = eal.does_asset_exist(MESH_DIR + "/" + name)
        meshes = [a for a in import_file(os.path.join(SRC, fname), MESH_DIR, name) if isinstance(a, unreal.StaticMesh)]
        if not meshes:
            log("FAILED mesh " + fname + " (no static mesh produced)")
            continue
        if not existed:
            new_files.append(fname)

        # Material instance.
        mi_path = MAT_DIR + "/MI_" + short
        if eal.does_asset_exist(mi_path):
            mi = unreal.load_asset(mi_path)
        else:
            mi = tools.create_asset("MI_" + short, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        found = 0
        for kind, param in (("base_color", "BaseColor"), ("normal", "Normal"), ("orm", "ORM")):
            tex = find_texture(short, kind)
            if tex:
                mel.set_material_instance_texture_parameter_value(mi, param, tex)
                found += 1
        eal.save_loaded_asset(mi)

        for mesh in meshes:
            size = normalize_scale(mesh)
            # GLBs with embedded textures (e.g. Tripo exports) keep their imported materials
            # unless separate T_<Name>_* textures were provided.
            if found:
                for i in range(len(mesh.static_materials)):
                    mesh.set_material(i, mi)
            else:
                log("  no T_%s_* textures: keeping the mesh's own (embedded) materials" % short)
            eal.save_loaded_asset(mesh)
            log("Mesh %s -> %s (MI_%s, ~%.0f cm)" % (fname, mesh.get_name(), short, size))

if new_files:
    with open(os.path.join(ROOT, "ASSETS.md"), "a", encoding="utf-8") as f:
        for fname in new_files:
            f.write("| ArtSource/%s | TODO source (made during jam? tool?) | TODO license | TODO |\n" % fname)
    log("Added %d ASSETS.md stub rows - fill in source and license!" % len(new_files))
log("DONE")
