A **2D tile-based game engine** specifically designed for the game **Rage of the Mage**.

Features
--------
  - Graphical tilemap editor
  - Chunk based tilemaps, fast binary save format
  - Arbitrary tileset support, configurable size and valid/usable tiles, serialized configuration
  - Flexible state machine handling (CoreModule)
  - Automatic reference-counted resource management (currently limited to textures)
  - Rich logging API: tracks function names, timestamps
  - Versatile Dear ImGui integration (CoreModule::sync_gui)
  - ...and more...

Compilation
-----------
Currently only supports MSYS2 UCRT64 toolchain for Windows.

Requires GCC 16+ and GNU Make. All external code is built at compilation time and linked to the final executable.

Navigate to the project directory and run ```mingw32-make MODE=release```. Other modes: debug, profile (Tracy support)

To run the application, execute ```./bin/[MODE]/rotm.exe``` or ```mingw32-make run MODE=release```.
