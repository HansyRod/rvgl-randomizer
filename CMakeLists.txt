cmake_minimum_required(VERSION 3.20)
project(RVGLRandomizer)

# ============================================================================
# Require x64. The mod targets 64-bit RVGL binaries and the hook backends are
# selected for x86-64. Refuse to configure if the wrong generator or toolchain
# is selected.
# ============================================================================
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "RVGL Randomizer must be built as x64. "
                        "Use -A x64 or select the x64 generator in CMake Tools.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Sub-projects
# ============================================================================
add_subdirectory(DLL)
add_subdirectory(Launcher)
