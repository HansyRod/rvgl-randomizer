================================================================================
RVGL RANDOMIZER — PROJECT STRUCTURE & BUILD GUIDE
================================================================================


--------------------------------------------------------------------------------
PREREQUISITES
--------------------------------------------------------------------------------

1. Visual Studio Build Tools 2022
   https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
   During install, select "Desktop development with C++"
   
   or
   
   Visual Studio 2022 (any edition — Community, Professional, or Enterprise)
   If you already have VS 2022 installed you do not need anything else.
   Make sure "Desktop development with C++" was selected during installation.

2. CMake 3.20 or later
   https://cmake.org/download/
   Add to PATH during install when prompted.

3. VS Code extensions (install from the Extensions panel):
   - C/C++        (Microsoft)
   - CMake Tools  (Microsoft)


--------------------------------------------------------------------------------
PROJECT LAYOUT
--------------------------------------------------------------------------------

rvgl-randomizer/
├── CMakeLists.txt              Root build file
│
├── DLL/                        The injected DLL (randomizer.dll)
│   ├── CMakeLists.txt
│   ├── Core/                   Hook infrastructure and DLL entry point
│   │   ├── dllmain.cpp
│   │   ├── HookManager.h/.cpp
│   │   └── Addresses.h
│   ├── Game/                   RVGL memory layout — structs and constants
│   │   └── RVGLStructs.h
│   ├── Mods/                   Individual mod logic
│   │   ├── Randomizer.h/.cpp
│   │   └── (future mods here)
│   └── MinHook/                MinHook static library (vendored)
│       ├── include/MinHook.h
│       └── src/...
│
├── Launcher/                   Test launcher executable
│   ├── CMakeLists.txt
│   ├── Launcher.h/.cpp         Launch + injection logic
│   └── main.cpp                CLI wrapper for development testing
│
└── .vscode/
    ├── settings.json           CMake Tools configuration
    ├── c_cpp_properties.json   IntelliSense include paths
    └── launch.json             Debug configurations


--------------------------------------------------------------------------------
FIRST-TIME SETUP IN VS CODE
--------------------------------------------------------------------------------

1. Open the rvgl-randomizer folder in VS Code:
       File → Open Folder → select rvgl-randomizer/

2. VS Code will prompt "Would you like to configure this project?" — click Yes.
   CMake Tools will run the configure step automatically.

   If it asks you to select a kit, choose:
       Visual Studio 2022 Release - amd64
   "amd64" means both the compiler and the target are x64.

3. Update the compiler path in .vscode/c_cpp_properties.json.
   The version number in the path (e.g. 14.40.33807) differs between
   installations. Find yours by browsing to:
       C:\Program Files\Microsoft Visual Studio\2022\<Edition>\
           VC\Tools\MSVC\<version>\bin\HostX64\x64\cl.exe
   Update the "compilerPath" field in c_cpp_properties.json to match.

4. Update the RVGL path in .vscode/launch.json.
   Replace "C:\\path\\to\\your\\rvgl.exe" in the "Debug TestLauncher"
   configuration with the actual path on your machine.


--------------------------------------------------------------------------------
BUILDING
--------------------------------------------------------------------------------

From VS Code:
    Press Ctrl+Shift+P → "CMake: Build" → Enter
    Or click the Build button in the CMake Tools status bar at the bottom.

From a terminal:
    cd rvgl-randomizer
    cmake -B build -A x64
    cmake --build build --config Debug

Output files:
    build/DLL/Debug/randomizer.dll
    build/Launcher/Debug/TestLauncher.exe

The post-build step in DLL/CMakeLists.txt automatically copies
randomizer.dll next to TestLauncher.exe after each build.


--------------------------------------------------------------------------------
TESTING
--------------------------------------------------------------------------------

From a terminal:
    build\Launcher\Debug\TestLauncher.exe "C:\path\to\rvgl.exe"

With extra RVGL arguments:
    build\Launcher\Debug\TestLauncher.exe "C:\path\to\rvgl.exe" "-window 1920 1080"

Expected output:
    [TestLauncher] RVGL : C:\path\to\rvgl.exe
    [TestLauncher] DLL  : ...\build\Launcher\Debug\randomizer.dll
    [TestLauncher] OK — RVGL running (PID 12345)

To see hook output from inside the DLL, run DebugView (Sysinternals) as
administrator before launching. Messages appear as:
    [HookManager] OK: LoadAllCars
    [HookManager] OK: SetupAllRaceCars
    [HookManager] InstallAll complete — 2 hook(s) active
    [Randomizer] Car pool snapshot — 49 cars


--------------------------------------------------------------------------------
DEBUGGING IN VS CODE
--------------------------------------------------------------------------------

Two debug configurations are provided in .vscode/launch.json:

"Debug TestLauncher"
    Runs TestLauncher.exe under the debugger. Use this to step through
    Launcher.cpp and verify injection is working correctly.
    Builds the project automatically before launching.

"Attach to RVGL"
    Attaches to a running rvgl.exe after injection. Use this to set
    breakpoints inside the mod DLL (Randomizer.cpp, HookManager.cpp etc.)
    Workflow:
        1. Launch RVGL via "Debug TestLauncher" or TestLauncher.exe directly
        2. Switch to "Attach to RVGL" in the debug panel and press F5
    Note: the DLL must be built in Debug configuration for breakpoints to work.


--------------------------------------------------------------------------------
ADDING A NEW HOOK
--------------------------------------------------------------------------------

1. Add the RVA constant to DLL/Core/Addresses.h
2. Declare Hook_Foo and Orig_Foo in the relevant Mods/ header
3. Define them in the corresponding Mods/ source file
4. Add one HookManager::Add() call in HookManager.cpp → RegisterHooks()

Nothing else changes.


--------------------------------------------------------------------------------
ADDING A NEW MOD
--------------------------------------------------------------------------------

1. Create DLL/Mods/MyMod.h and DLL/Mods/MyMod.cpp
2. Add MyMod.cpp to the source list in DLL/CMakeLists.txt
3. Add #include "MyMod.h" in HookManager.cpp
4. Add the relevant HookManager::Add() calls in RegisterHooks()


--------------------------------------------------------------------------------
FIRST-TIME SETUP AFTER CLONING
--------------------------------------------------------------------------------
 
The .vscode/ configuration files are machine-specific and not committed to the
repository. Template versions are provided in .vscode/templates/.
 
Copy each template and fill in the placeholders:
 
    copy .vscode\templates\settings.json        .vscode\settings.json
    copy .vscode\templates\c_cpp_properties.json .vscode\c_cpp_properties.json
    copy .vscode\templates\launch.json           .vscode\launch.json
 
Then:
  - In c_cpp_properties.json: replace <Edition> and <version> in compilerPath
  - In launch.json: replace the rvgl.exe path in "Debug TestLauncher" args