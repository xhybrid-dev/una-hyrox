# HybridX Trail -- breadcrumb route navigation in a run-recording app, with
# its GUI process built on LVGL.
#
# Started from the SDK's RunLVGL (Examples/Apps/RunLVGL/Software/Apps/
# RunLVGL-CMake/CMakeLists.txt), as the standing rules require. The
# differences: the name and ID, the Libs split into Core / App
# (../../Libs/libs.cmake), and the version taken from trail-v* tags
# (../../cmake/trail-version.cmake).
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "HybridXTrail")
set(APP_USER_NAME "HybridX Trail")      # 13 bytes; app_merging.py truncates at 15
set(APP_FILE_NAME "HybridXTrail")
set(APP_TYPE "Activity")
set(DEV_ID "HybridX")
set(APP_ID "71ABD15ED7526601")          # first 16 hex of md5("HybridXTrail"); development ID
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LVGL-GUI")
# Output/ is git-ignored; the linker needs it to exist for its .map files.
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
include(${LIBS_PATH}/libs.cmake)        # LIBS_SOURCES, LIBS_INCLUDE_DIRS
include(${GUI_PATH}/lvgl-gui.cmake)     # GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS

project(${APP_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/trail-version.cmake)
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Service: the route core and the app's service.
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

# GUI (LVGL). It also compiles the route core, for the map projection.
set(GUI_SOURCES
    ${GUI_APP_SOURCES}
    ${TRAIL_CORE_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI_LVGL}
)
set(GUI_INCLUDE_DIRS
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
    ${GUI_APP_INCLUDE_DIRS}
)
set(GUI_COMPILE_DEFINITIONS
    ${UNA_SDK_DEFINES_GUI_LVGL}
)
# LVGL's software renderer recurses deeper than TouchGFX (RunLVGL's figures).
set(GUI_STACK_SIZE "24*1024")
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

una_app_build_app()
