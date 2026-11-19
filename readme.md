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
Currently supports Windows (MSYS2 UCRT64 gcc) and Ubuntu-based Linux desktops (X11 only).

Requires GCC and GNU Make.

For Windows:
  - Visit https://msys2.org, download and run the installer.
  - After installation, open "MSYS2 UCRT64".
  - Run the following command to install GCC: ```pacman -S mingw-w64-ucrt-x86_64-gcc```
  - Run the following command to install GNU Make: ```pacman -S mingw-w64-ucrt-x86_64-make```
  - Go to Settings->System->About->Advanced system settings.
  - Select "Environment Variables...".
  - Click on Path under "User variables for [username]".
  - Select "Edit...".
  - Select "New" and enter "[MSYS2 install location]\ucrt64\bin".
  - For all following instructions, replace "make" with "mingw32-make".

For Linux:
  - Run the following command to install the build-essential package: ```sudo apt install build-essential```. Enter password when prompted.
  - Run the following command to install raylib's dependencies: ```sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev```

Navigate to the project directory in your terminal and run ```make MODE=release```. Other modes: debug, profile (Tracy support)

To speed up compilation, you can call ```make -jx```, where x is the number of threads that your computer supports.

To run the application, execute ```./bin/[MODE]/rotm.exe``` or ```make run MODE=[MODE]```.
