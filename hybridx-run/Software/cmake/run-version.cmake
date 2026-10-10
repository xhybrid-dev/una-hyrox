# Stamp BUILD_VERSION from this app's own tag family, run-vX.Y.Z.
#
# una-app.cmake's una_app_setup_version() hard-codes the apps- prefix (line 149),
# which in this repository belongs to HybridX Race. It does honour a
# BUILD_VERSION defined before it is called (lines 141-143), so this sets one.
#
# una-version.sh normalises only apps-v / sdk-v / v, so "run-v1.2.0" comes
# back verbatim and app_merging.py would reject it: strip the prefix here, and
# fall back to 0.0.0-dev for anything that is not X.Y.Z (untagged or dirty
# trees), as the script itself does for apps-.
#
# A -DBUILD_VERSION=... on the command line still wins.
if(NOT DEFINED BUILD_VERSION)
    execute_process(
        COMMAND bash $ENV{UNA_SDK}/Utilities/Scripts/build-cube/una-version.sh
                ${CMAKE_CURRENT_SOURCE_DIR} run-
        OUTPUT_VARIABLE _run_version_out
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "BUILD_VERSION=([^\n]+)$" _unused "${_run_version_out}")
    string(REGEX REPLACE "^run-v" "" _run_version "${CMAKE_MATCH_1}")
    if(NOT _run_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        set(_run_version "0.0.0-dev")
    endif()
    set(BUILD_VERSION "${_run_version}")
    message("HybridX Run BUILD_VERSION: ${BUILD_VERSION} (from run-v* tags)")
endif()
