cmake_minimum_required(VERSION 3.20)

# ============================================================================
# TestLauncher.exe
#
# Minimal console executable for testing injection during development.
# In production this will be replaced by the UI calling Launcher::Launch()
# directly.
#
# Usage:
#   TestLauncher.exe "C:\path\to\rvgl.exe"
#   TestLauncher.exe "C:\path\to\rvgl.exe" "-window 1920 1080"
# ============================================================================
add_executable(TestLauncher
    Launcher.cpp
    main.cpp
)

target_include_directories(TestLauncher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TestLauncher PRIVATE kernel32)

set_target_properties(TestLauncher PROPERTIES OUTPUT_NAME "TestLauncher")
