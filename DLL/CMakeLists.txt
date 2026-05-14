cmake_minimum_required(VERSION 3.20)

# ============================================================================
# nlohmann_json — content fetched from the public repository
#                 and linked into the mod DLL.
# ============================================================================

# 1. Include the FetchContent module
include(FetchContent)

# 2. Declare the dependency using the minimal release tarball
FetchContent_Declare(
    nlohmann_json
    URL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

# 3. Make the targets available
FetchContent_MakeAvailable(nlohmann_json)

# ============================================================================
# MinHook — compiled as a static library and linked into the mod DLL.
# ============================================================================
add_library(MinHook STATIC
    MinHook/src/buffer.c
    MinHook/src/hook.c
    MinHook/src/trampoline.c
    MinHook/src/hde/hde32.c
    MinHook/src/hde/hde64.c
)

target_include_directories(MinHook PUBLIC
    MinHook/include
)

# Suppress warnings on vendored code we are not responsible for maintaining.
target_compile_options(MinHook PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W0>
)

# ============================================================================
# randomizer.dll
# ============================================================================
add_library(randomizer SHARED
    # Core — hook infrastructure, DLL entry point
    Core/dllmain.cpp
    Core/HookManager.cpp

    # Mods — individual mod logic
    Mods/RandomizerState.cpp
    Mods/RandomizerInit.cpp
    Mods/CarHooks.cpp
    Mods/Carbox.cpp
    Mods/TrackHooks.cpp
    Mods/CupHooks.cpp
    Mods/ProfileHooks.cpp
    Mods/CarPhysicsHooks.cpp
    Mods/RaceInitHooks.cpp
    Mods/30CarMod.cpp

    # DebugUtils — debug tools
    DebugUtils/Logger.cpp        # OutputDebugStringA + file logging
    DebugUtils/CallLogger.cpp    # auto-generated passthrough hooks from map

    # Image - Image manipulation logic that interacts with the stb libraries
    Image/Image.cpp

    # Game - Game data or other utils
    Game/GameUtils.cpp

    #Config - Handles loading the JSON configuration with the randomization data
    Config/ConfigManager.cpp

    #File - Handles loading files from the mod folder and packlist overrides
    File/FileUtils.cpp

)

target_include_directories(randomizer PRIVATE
    # Each subfolder is on the include path so files can use
    # #include "HookManager.h", #include "RVGLStructs.h" etc.
    # without needing to know where they live.
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Game
    ${CMAKE_CURRENT_SOURCE_DIR}/Mods
    ${CMAKE_CURRENT_SOURCE_DIR}/DebugUtils
    ${CMAKE_CURRENT_SOURCE_DIR}/Image
    ${CMAKE_CURRENT_SOURCE_DIR}/Image/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/File
    MinHook/include
)

target_link_libraries(randomizer PRIVATE
    MinHook
    kernel32
    nlohmann_json::nlohmann_json
)

set_target_properties(randomizer PROPERTIES OUTPUT_NAME "randomizer")

# ============================================================================
# Post-build: copy the DLL next to the test launcher so it can be found
# without manually moving files after every build.
# ============================================================================
add_custom_command(TARGET randomizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:randomizer>
        ${CMAKE_BINARY_DIR}/Launcher/$<CONFIG>/randomizer.dll
    COMMENT "Copying randomizer.dll next to TestLauncher"
)
