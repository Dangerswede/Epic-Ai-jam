"""
Sets up the audio data asset and imports sounds.

1. Creates (once) in /Game/Retake/Audio: SC_Music, SC_SFX, SC_Voice (sound classes), SCM_RetakeVolume
   (sound mix used by the menu volume sliders) and DA_RetakeAudio (URetakeAudioSet).
2. Imports every .wav/.ogg/.mp3 in AudioSource/ whose file name equals a slot name
   (e.g. VoiceAction.wav, MusicRollingLoop.wav, SfxExplosion.wav) and assigns it to that slot.
   Music* files (except MusicDirectorsCut) are set to loop. Unknown names are imported but not assigned.
3. Appends an ASSETS.md row stub for every newly imported file.

Run headless:
  UnrealEditor-Cmd.exe EpicAiJam.uproject -run=pythonscript -script="<path>/import_audio.py"
"""
import os
import re
import unreal

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals()
                                    else r"D:\Projekt\EpicAiJam\Scripts", ".."))
SRC = os.path.join(ROOT, "AudioSource")
DIR = "/Game/Retake/Audio"
SOUND_DIR = DIR + "/Sounds"
DA_PATH = DIR + "/DA_RetakeAudio"

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


def log(msg):
    print("[import_audio] " + msg)
    unreal.log("[import_audio] " + msg)


def get_or_create(name, cls, factory):
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        return unreal.load_asset(path)
    asset = tools.create_asset(name, DIR, cls, factory)
    eal.save_loaded_asset(asset)
    log("Created " + path)
    return asset


def snake(name):
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name).lower()


sc_music = get_or_create("SC_Music", unreal.SoundClass, unreal.SoundClassFactory())
sc_sfx = get_or_create("SC_SFX", unreal.SoundClass, unreal.SoundClassFactory())
sc_voice = get_or_create("SC_Voice", unreal.SoundClass, unreal.SoundClassFactory())
mix = get_or_create("SCM_RetakeVolume", unreal.SoundMix, unreal.SoundMixFactory())

if eal.does_asset_exist(DA_PATH):
    audio_set = unreal.load_asset(DA_PATH)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RetakeAudioSet)
    audio_set = tools.create_asset("DA_RetakeAudio", DIR, unreal.RetakeAudioSet, factory)
    log("Created " + DA_PATH)
audio_set.set_editor_property("volume_mix", mix)
audio_set.set_editor_property("music_class", sc_music)
audio_set.set_editor_property("sfx_class", sc_sfx)
audio_set.set_editor_property("voice_class", sc_voice)

# --- import sounds ---------------------------------------------------------------------------
new_files = []
if os.path.isdir(SRC):
    for fname in sorted(os.listdir(SRC)):
        base, ext = os.path.splitext(fname)
        if ext.lower() not in (".wav", ".ogg", ".mp3", ".flac"):
            continue
        dest = SOUND_DIR + "/" + base
        existed = eal.does_asset_exist(dest)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(SRC, fname))
        task.set_editor_property("destination_path", SOUND_DIR)
        task.set_editor_property("destination_name", base)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tools.import_asset_tasks([task])
        sound = unreal.load_asset(dest)
        if not sound:
            log("FAILED to import " + fname)
            continue
        if not existed:
            new_files.append(fname)
        if base.startswith("Music") and base != "MusicDirectorsCut" and isinstance(sound, unreal.SoundWave):
            sound.set_editor_property("looping", True)
            eal.save_loaded_asset(sound)
        prop = snake(base)
        try:
            audio_set.set_editor_property(prop, sound)
            log("Assigned %s -> %s" % (fname, prop))
        except Exception:
            log("Imported %s (no slot named %s - assign it by hand)" % (fname, prop))
else:
    log("No AudioSource folder at " + SRC)

eal.save_loaded_asset(audio_set)

# --- ASSETS.md stubs -------------------------------------------------------------------------
if new_files:
    with open(os.path.join(ROOT, "ASSETS.md"), "a", encoding="utf-8") as f:
        for fname in new_files:
            f.write("| AudioSource/%s | TODO source (made during jam?) | TODO license | TODO |\n" % fname)
    log("Added %d ASSETS.md stub rows - fill in source and license!" % len(new_files))
log("DONE")
