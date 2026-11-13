"""
F3: explosion FX. Copies the engine's Niagara "SimpleExplosion" template (fireball, smoke, sparks)
into /Game/Retake/FX/NS_Explosion so the game owns (and cooks) its own copy.
Run: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=Scripts/setup_fx.py -unattended -nop4 -nosplash -NoSound
"""
import unreal

SRC = "/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion"
DST_DIR = "/Game/Retake/FX"
DST = DST_DIR + "/NS_Explosion"

lib = unreal.EditorAssetLibrary
if not lib.does_directory_exist(DST_DIR):
    lib.make_directory(DST_DIR)
if not lib.does_asset_exist(DST):
    src = unreal.load_asset(SRC)  # plugin content isn't in the commandlet's asset registry yet
    if not src or not unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset("NS_Explosion", DST_DIR, src):
        unreal.log_error("FX: could not duplicate " + SRC)
system = unreal.load_asset(DST)
unreal.log("FX: loaded %s" % system)
try:
    for h in system.get_editor_property("emitter_handles"):
        unreal.log("FX emitter: %s" % h.get_editor_property("name"))
except Exception as e:  # property access differs between versions; listing is only informative
    unreal.log("FX: could not list emitters (%s)" % e)
lib.save_asset(DST)
unreal.log("FX: saved " + DST)

# Fireball glow: additive, unlit, hot in the middle (1 - fresnel), fades with the Intensity parameter.
GLOW = DST_DIR + "/M_FxGlow"
if not lib.does_asset_exist(GLOW):
    mel = unreal.MaterialEditingLibrary
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_FxGlow", DST_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(8.0, 3.0, 0.6, 1.0))
    inten = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 250)
    inten.set_editor_property("parameter_name", "Intensity")
    inten.set_editor_property("default_value", 1.0)
    fres = mel.create_material_expression(m, unreal.MaterialExpressionFresnel, -800, 400)
    fres.set_editor_property("exponent", 1.5)
    core = mel.create_material_expression(m, unreal.MaterialExpressionOneMinus, -550, 400)
    mel.connect_material_expressions(fres, "", core, "")
    mul1 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -400, 100)
    mel.connect_material_expressions(color, "", mul1, "A")
    mel.connect_material_expressions(inten, "", mul1, "B")
    mul2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -200, 200)
    mel.connect_material_expressions(mul1, "", mul2, "A")
    mel.connect_material_expressions(core, "", mul2, "B")
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    lib.save_asset(GLOW)
    unreal.log("FX: created " + GLOW)

# Projection screen (scene 13): unlit, emissive = captured texture * warm tint * Brightness.
SCREEN = DST_DIR + "/M_ProjectionScreen"
if not lib.does_asset_exist(SCREEN):
    mel = unreal.MaterialEditingLibrary
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_ProjectionScreen", DST_DIR, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    tex.set_editor_property("parameter_name", "Screen")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
    tint = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 0.88, 0.7, 1.0))
    bright = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 400)
    bright.set_editor_property("parameter_name", "Brightness")
    bright.set_editor_property("default_value", 3.0)
    mul1 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -500, 100)
    mel.connect_material_expressions(tex, "RGB", mul1, "A")
    mel.connect_material_expressions(tint, "", mul1, "B")
    mul2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 200)
    mel.connect_material_expressions(mul1, "", mul2, "A")
    mel.connect_material_expressions(bright, "", mul2, "B")
    mel.connect_material_property(mul2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    lib.save_asset(SCREEN)
    unreal.log("FX: created " + SCREEN)
