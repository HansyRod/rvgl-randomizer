cmake_minimum_required(VERSION 3.20)

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
    Mods/Randomizer.cpp
)

target_include_directories(randomizer PRIVATE
    # Each subfolder is on the include path so files can use
    # #include "HookManager.h", #include "RVGLStructs.h" etc.
    # without needing to know where they live.
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Game
    ${CMAKE_CURRENT_SOURCE_DIR}/Mods
    MinHook/include
)

target_link_libraries(randomizer PRIVATE
    MinHook
    kernel32
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
