# Lotus

A 3D DirectX 12 game engine written in C++ inspired by Primal. The Lotus Editor is written in C# and will always only support windows. But as the game is built from the project, rather than the editor, it is possible games can be made for non-windows platform in the future if I ever implement a graphics API other than DirectX.

## Working with the code
At this time, the engine and the editor both require Windows 10 or 11 and a GPU that supports DirectX 12. In addition, in order to build ContentTools, Autodesk FBX SDK `2020.2` must be installed on the device.
In order to not have to modify project settings, ensure the include and library folders can be found at `C:\Program Files\Autodesk\FBX\FBX SDK\2020.2`

Suggested to add command line option `/ignore:4099` to linker options to supress warnings about missing FBX PDBs.

### Roadmap
___
This is a rough idea of what to expect. No ETA is given for anything. As of January 16th, 2023 I am still currently working on getting DirectX 12 supported and ready to use.
___
#### Lotus Engine (the C++ side)
 + [X] Window handling, including support for multiple windows 
 + [X] generational ID system
 + [X] ID integer based entity component system
 + [X] C++ native scripting
 + [ ] Direct3D 12 support
   + In progress
 + [ ] Lotus managed pointer/memory types
 + [X] Lotus managed lists and vectors
   + Vectors and free lists are implemented currently
 + [ ] Textures
 + [ ] Forward Lighting
 + [X] HLSL shaders
   + With shader model 6.6, Dynamic Resources are currently bugged for recent drivers for RTX 3000 series and 4090
 + [X] Models and Meshes
 + [ ] Animation
 + [ ] Shadows
 + [ ] Physics
 + [ ] User input
 + [X] Cameras
 + [X] Forward+ rendering
 + [ ] Material system
 + [ ] Physically Based Rendering - PBR
 + [ ] Image based lighting
 + [ ] Multithreading
 + [ ] Logging communication with the editor
 + [ ] Particle systems
 + [ ] Font rendering
 + [ ] HDR support
 + [ ] Antialiasing
 + [ ] Screen space reflections
 + [ ] Ambient occlusion
 + [ ] More post processing effects
 + [ ] Controller input
 + [ ] Audio
 + [ ] Bloom
 + [ ] Tone mapping
___
#### Lotus Editor (The C# side)
+ [ ] Geometry editor
  + Partially implemented
+ [X] Project open/create dialog
+ [X] Win32 window hosting
+ [X] Content/asset browser
+ [X] Model importer
+ [ ] Texture editor
+ [ ] Material editor
+ [ ] Lights
+ [ ] Gizmos (rotating, scaling, translating)
+ [ ] Animation editor
+ [ ] Audio import and player
+ [ ] Profiling tools