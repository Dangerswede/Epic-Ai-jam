ArtSource - drop raw art here, then run Scripts\import_art.py (or ask Claude to).

Run (editor closed):
  "E:\Unreal engine\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" D:\Projekt\EpicAiJam\EpicAiJam.uproject -ExecutePythonScript="D:\Projekt\EpicAiJam\Scripts\import_art.py" -unattended -nullrhi

NAMING
  Meshes    SM_<Name>.glb | .gltf | .fbx | .obj      e.g. SM_StudioSpotlight.glb, SM_Clapperboard.fbx
  Textures  T_<Name>_BaseColor.png                  colour (sRGB)
            T_<Name>_Normal.png                     normal map (DirectX/UE green-down if possible)
            T_<Name>_ORM.png                        R = ambient occlusion, G = roughness, B = metallic
  <Name> of the textures must match the mesh name, e.g. SM_Plywood.glb + T_Plywood_BaseColor.png.
  Also accepted: _Albedo/_Diffuse/_Color, _N, _ARM. .jpg/.tga work too.

WHAT THE SCRIPT DOES
  - Imports meshes to Content/Retake/Art/Meshes (SM_), textures to Content/Retake/Art/Textures (T_).
  - Creates MI_<Name> from M_RetakeMaster (params BaseColor, BaseColorTint, Normal, ORM,
    RoughnessScale, MetallicScale), plugs in the matching textures and puts it on the mesh.
  - Scale check: a mesh smaller than 5 cm is assumed to be in meters and scaled x100;
    larger than 50 m is scaled x0.01. Check the log. Real-world size: 1 unit = 1 cm.
  - Adds a row stub per new file to ASSETS.md -> fill in source/tool and license!

TIPS
  - Pivot at the bottom center for props that stand on the floor.
  - Keep each prop to one material where possible (one MI per mesh).
  - Swap a gameplay actor's look: select it in the level -> Details -> its mesh component
    (PlateMesh, DoorMesh, BarrelMesh, CrateMesh, BodyMesh...) -> pick the SM_.
  - Rules: no third-party IP, made during the jam (or listed in ASSETS.md with license).
