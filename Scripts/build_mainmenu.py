"""
Creates /Game/Retake/Maps/MainMenu: an empty map whose World Settings use RetakeMenuGameMode
(no pawn, call sheet menu). Run headless like build_scene01.py.
"""
import unreal

MAP_PATH = "/Game/Retake/Maps/MainMenu"
eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

if eal.does_asset_exist(MAP_PATH):
    eal.delete_asset(MAP_PATH)
if not les.new_level(MAP_PATH):
    raise RuntimeError("new_level failed")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.RetakeMenuGameMode)

les.save_current_level()
unreal.log("[build_mainmenu] Saved " + MAP_PATH)
