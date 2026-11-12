"""
F4: imports the 70s display font (SIL OFL, from github.com/google/fonts) as a font face and
points URetakeSettings.PopFontFace at it.
Put the .ttf in ArtSource/Fonts first. Run:
UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=Scripts/import_font.py -unattended -nop4 -nosplash -NoSound
"""
import os
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
SRC = os.path.join(ROOT, "ArtSource", "Fonts", "Shrikhand-Regular.ttf")
DST_DIR = "/Game/Retake/UI/Fonts"
NAME = "FF_Shrikhand"

task = unreal.AssetImportTask()
task.set_editor_property("filename", SRC)
task.set_editor_property("destination_path", DST_DIR)
task.set_editor_property("destination_name", NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

face = unreal.load_asset(DST_DIR + "/" + NAME)
if not isinstance(face, unreal.FontFace):
    unreal.log_error("FONT: import did not create a FontFace: %s" % face)
else:
    face.set_editor_property("loading_policy", unreal.FontLoadingPolicy.INLINE)  # data inside the asset: safe when cooked
    unreal.EditorAssetLibrary.save_asset(face.get_path_name())
    unreal.log("FONT: imported " + face.get_path_name())
