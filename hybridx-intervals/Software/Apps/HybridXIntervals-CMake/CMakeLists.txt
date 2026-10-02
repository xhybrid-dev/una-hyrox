# HybridX Intervals -- structured workouts (warm-up, work and rest steps with
# pace or heart-rate targets, repeats, cool-down) in a run-recording app, with
# its GUI process built on LVGL.
#
# Started from the SDK's RunLVGL (Examples/Apps/RunLVGL/Software/Apps/
# RunLVGL-CMake/CMakeLists.txt), as the standing rules require. The
# differences: the name and ID, the Libs split into Core / App
# (../../Libs/libs.cmake), and the version taken from intervals-v* tags
# (../../cmake/intervals-version.cmake).
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "HybridXIntervals")
set(APP_USER_NAME "HX Intervals")       # 12 bytes; app_merging.py truncates at 15
set(APP_FILE_NAME "HybridXIntervals")
set(APP_TYPE "Activity")
set(DEV_ID "HybridX")
set(APP_ID "DBDC6FEA92394563")          # first 16 hex of md5("HybridXIntervals"); development ID
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LVGL-GUI")
# Output/ is git-ignored; the linker needs it to exist for its .map files.
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Run SDK/Utilities/Scripts/set-una-sdk-env.ps1 or set UNA_SDK to the SDK root.")
endif()

# Includes all build steps and required tools
# Sets toolchain before project
include($ENV{UNA_SDK}/cmake/una-app.cmake)

# Includes sets: SCRIPTS_PATH,
# UNA_SDK_SOURCES_COMMON, UNA_SDK_SOURCES_SERVICE, UNA_SDK_SOURCES_GUI_LVGL,
# UNA_SDK_INCLUDE_DIRS_COMMON, UNA_SDK_INCLUDE_DIRS_SERVICE, UNA_SDK_INCLUDE_DIRS_GUI_LVGL,
# UNA_SDK_DEFINES_GUI_LVGL
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)

# Includes LIBS_SOURCES, LIBS_INCLUDE_DIRS
include(${LIBS_PATH}/libs.cmake)

# Includes GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS
include(${GUI_PATH}/lvgl-gui.cmake)

project(${APP_NAME})

# Set up version: from intervals-v* tags, not the SDK's apps-v*.
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/intervals-version.cmake)
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Build service
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

# Build GUI (LVGL). It also compiles the workout core, for the text it shows
# (WorkoutText, TextFold).
set(GUI_SOURCES
    ${GUI_APP_SOURCES}
    ${INTERVALS_CORE_SOURCES}
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
# Frame timing in the syslog, one line per 100 kernel ticks (see the LVGL port).
# For measuring, not for shipping: cmake -DRUNLVGL_FRAME_STATS=ON ...
option(RUNLVGL_FRAME_STATS "Log frame timing statistics from the GUI process" OFF)
if(RUNLVGL_FRAME_STATS)
    list(APPEND GUI_COMPILE_DEFINITIONS UNA_LVGL_FRAME_STATS=1)
endif()
# LVGL's software renderer recurses deeper than TouchGFX; give the GUI task
# more stack than the 10 KB default.
set(GUI_STACK_SIZE "24*1024")
# The GUI image (LVGL + fonts + icons + screens) is larger than a TouchGFX one;
# the default 600K linker region is too tight. The kernel allocates only what
# the image actually needs, so this is a ceiling, not a reservation.
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

# Build the complete app using SDK functions
una_app_build_app()
