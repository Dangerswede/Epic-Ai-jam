"""
C1: sets up the stunt double character from the engine mannequin pack copied to /Game/Characters/Mannequins.
- Keeps only what SKM_Manny_Simple + ABP_Unarmed actually reference (LFS quota); deletes the rest of the pack.
- Marks M_RetakeMaster and M_Ghost as usable on skeletal meshes.
- Creates MI_StuntSuit_Red / MI_StuntSuit_White (children of M_RetakeMaster).
Run (editor closed):
  UnrealEditor-Cmd.exe EpicAiJam.uproject -run=pythonscript -script="<path>/setup_character.py" -AllowCommandletRendering
"""
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
reg = unreal.AssetRegistryHelpers.get_asset_registry()

PACK = "/Game/Characters/Mannequins"
KEEP_ROOTS = [PACK + "/Meshes/SKM_Manny_Simple", PACK + "/Anims/Unarmed/ABP_Unarmed"]
SUIT_DIR = "/Game/Retake/Art/Materials"


def log(msg):
    print("[setup_character] " + msg)
    unreal.log("[setup_character] " + msg)


# --- 1. dependency closure inside the pack -------------------------------------------------
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                             include_searchable_names=False, include_soft_management_references=False,
                                             include_hard_management_references=False)
keep = set()
todo = list(KEEP_ROOTS)
while todo:
    pkg = todo.pop()
    if pkg in keep:
        continue
    keep.add(pkg)
    for dep in reg.get_dependencies(unreal.Name(pkg), opts) or []:
        d = str(dep)
        if d.startswith("/Game/Characters") and d not in keep:
            todo.append(d)
log("keeping %d packages" % len(keep))
for k in sorted(keep):
    log("  keep " + k)

removed = 0
for path in eal.list_assets("/Game/Characters", recursive=True, include_folder=False):
    pkg = path.split(".")[0]
    if pkg not in keep:
        if eal.delete_asset(pkg):
            removed += 1
        else:
            log("  could not delete " + pkg)
log("removed %d unused packages" % removed)

# --- 2. materials usable on skeletal meshes -----------------------------------------------
for mpath in ("/Game/Retake/Art/Materials/M_RetakeMaster", "/Game/Retake/Materials/M_Ghost"):
    m = unreal.load_asset(mpath)
    if m:
        m.set_editor_property("used_with_skeletal_mesh", True)
        mel.recompile_material(m)
        eal.save_loaded_asset(m)
        log("skeletal usage on " + mpath)

# --- 3. stunt suit -------------------------------------------------------------------------
master = unreal.load_asset("/Game/Retake/Art/Materials/M_RetakeMaster")
for name, color, rough in (("MI_StuntSuit_Red", unreal.LinearColor(0.72, 0.04, 0.03, 1), 0.55),
                           ("MI_StuntSuit_White", unreal.LinearColor(0.9, 0.9, 0.86, 1), 0.6)):
    path = SUIT_DIR + "/" + name
    mi = unreal.load_asset(path) if eal.does_asset_exist(path) else \
        tools.create_asset(name, SUIT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    mel.set_material_instance_vector_parameter_value(mi, "BaseColorTint", color)
    mel.set_material_instance_scalar_parameter_value(mi, "RoughnessScale", rough)
    eal.save_loaded_asset(mi)
    log("suit material " + path)
log("DONE")
