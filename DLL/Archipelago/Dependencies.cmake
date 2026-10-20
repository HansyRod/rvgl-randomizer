# Initialize only missing dependencies so existing submodule checkouts are preserved.
function(rvgl_require_archipelago_dependency dependency required_file)
    set(dependency_path "DLL/Archipelago/third_party/${dependency}")
    set(dependency_source "${PROJECT_SOURCE_DIR}/${dependency_path}")
    if(EXISTS "${dependency_source}/${required_file}")
        return()
    endif()

    set(recovery_message
        "Run 'git submodule update --init --recursive' from the repository root. "
        "For a source archive, use a Git clone with --recurse-submodules instead.")
    find_package(Git QUIET)
    if(NOT GIT_FOUND OR NOT EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(FATAL_ERROR
            "Missing Archipelago dependency: ${dependency_path}/${required_file}. "
            "Automatic initialization requires Git and a Git checkout. "
            ${recovery_message})
    endif()

    message(STATUS "Initializing Archipelago dependency: ${dependency}")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive -- "${dependency_path}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        RESULT_VARIABLE dependency_result
        ERROR_VARIABLE dependency_error
    )
    if(NOT dependency_result STREQUAL "0" OR
       NOT EXISTS "${dependency_source}/${required_file}")
        message(FATAL_ERROR
            "Could not initialize Archipelago dependency: ${dependency}.\n"
            "${dependency_error}\n"
            ${recovery_message})
    endif()
endfunction()

rvgl_require_archipelago_dependency(apclientpp CMakeLists.txt)
rvgl_require_archipelago_dependency(wswrap include/wswrap.hpp)
rvgl_require_archipelago_dependency(asio asio/include/asio.hpp)
rvgl_require_archipelago_dependency(websocketpp websocketpp/client.hpp)
