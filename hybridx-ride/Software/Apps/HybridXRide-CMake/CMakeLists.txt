# HybridX Ride -- heart-rate and GPS rides that count in HybridX Streak, with
# its GUI process built on LVGL. See ../../../docs/NOTES.md.
#
# From HybridX Workout's build (itself X Sauna's, from Knitting's and the SDK's Waypoint tutorial).
# The differences: the name and ID, and the version taken from ride-v* tags
# (../../cmake/ride-version.cmake).
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "HybridXRide")
set(APP_USER_NAME "HybridX Ride")      # 15 bytes; app_merging.py truncates at 15
set(APP_FILE_NAME "HybridXRide")
set(APP_TYPE "Activity")                  # as HybridX Race and the SDK's Cycling
set(DEV_ID "HybridX")
set(APP_ID "250009C2B1049597")          # development ID: first 16 hex of md5("HybridXRide"); the portal's replaces it at release
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LVGL-GUI")
# Output/ is git-ignored; the linker needs it to exist for its .map files.
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
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

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/ride-version.cmake)
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

# Build GUI (LVGL). It sees the App and Core headers for the message contract
# and the header-only helpers, but links none of their sources.
set(GUI_SOURCES
    ${GUI_APP_SOURCES}
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
# LVGL's software renderer recurses deeper than TouchGFX, and the image is
# larger than the SDK defaults allow for; same values as HybridX Race and
# RunLVGL.
set(GUI_STACK_SIZE "24*1024")
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

# Build the complete app using SDK functions
una_app_build_app()
