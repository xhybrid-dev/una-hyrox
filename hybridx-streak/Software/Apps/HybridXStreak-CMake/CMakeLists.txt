# HybridX Streak -- a Utility app with an LVGL GUI.
#
# Laid out like hybridx-race/Software/Apps/HybridXRace-CMake. The differences:
# APP_TYPE Utility (it records no activity of its own), the Libs split into
# Core / App / Glance (see ../../Libs/libs.cmake), and the version taken from
# streak-v* tags (../../cmake/streak-version.cmake).
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "HybridXStreak")
set(APP_USER_NAME "HybridX Streak")     # 14 bytes; app_merging.py truncates at 15
set(APP_FILE_NAME "HybridXStreak")
set(APP_TYPE "Utility")
set(DEV_ID "HybridX")
set(APP_ID "A75434BA114DFEBE")          # first 16 hex of md5("HybridXStreak"); development ID
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
include(${LIBS_PATH}/libs.cmake)
include(${GUI_PATH}/lvgl-gui.cmake)

project(${APP_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/streak-version.cmake)
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Service
set(SERVICE_SOURCES
    ${STREAK_CORE_SOURCES}
    ${STREAK_APP_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${STREAK_APP_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

# GUI (LVGL). It sees the App headers for the message contract and the Core
# headers, but links none of their sources.
set(GUI_SOURCES
    ${GUI_APP_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI_LVGL}
)
set(GUI_INCLUDE_DIRS
    ${STREAK_APP_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
    ${GUI_APP_INCLUDE_DIRS}
)
set(GUI_COMPILE_DEFINITIONS
    ${UNA_SDK_DEFINES_GUI_LVGL}
    ${STREAK_GUI_DEFINES}
)
# LVGL's renderer recurses deeper than TouchGFX, and the image is larger than
# the SDK defaults allow for; same values as HybridX Race and RunLVGL.
set(GUI_STACK_SIZE "24*1024")
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

una_app_build_app()
