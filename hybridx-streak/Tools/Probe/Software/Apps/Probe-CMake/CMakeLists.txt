# Streak Probe -- the throwaway Phase S0 app that answers Gate 0: can one app
# list and read another app's Activity/*.fit on a real watch?
# (hybridx-streak PLAN section 3; how to run it: hybridx-streak/docs/PROBE.md)
#
# Built like the HybridX Streak app, with its own name and ID so it installs
# alongside it and can be deleted afterwards. Its version is fixed: it is never
# released, so it takes no streak-v tag.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "HXStreakProbe")
set(APP_USER_NAME "Streak Probe")
set(APP_FILE_NAME "HXStreakProbe")
set(APP_TYPE "Utility")
set(DEV_ID "HybridX")
set(APP_ID "DE9CF1F8FFF3776D")          # first 16 hex of md5("HXStreakProbe"); development ID
set(BUILD_VERSION "0.1.0")
set(STREAK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")
set(RESOURCES_PATH "${STREAK_ROOT}/Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(PROBE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Probe-GUI")
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)

project(${APP_NAME})
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

set(STREAK_FONTS "${STREAK_ROOT}/Software/Apps/LVGL-GUI/assets/fonts")

# Service: the checks, and the week maths (Core) for the clock line.
set(SERVICE_SOURCES
    ${PROBE_LIBS}/Sources/ProbeRunner.cpp
    ${PROBE_LIBS}/Sources/Service.cpp
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${PROBE_LIBS}/Header
    ${STREAK_ROOT}/Software/Libs/Core/Header
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

# GUI: one screen, three of the Streak app's fonts.
set(GUI_SOURCES
    ${GUI_PATH}/gui/src/ProbeGui.cpp
    ${STREAK_FONTS}/poppins_semibold_25.c
    ${STREAK_FONTS}/poppins_regular_16.c
    ${STREAK_FONTS}/poppins_regular_14.c
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI_LVGL}
)
set(GUI_INCLUDE_DIRS
    ${GUI_PATH}/gui/include
    ${PROBE_LIBS}/Header
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)
set(GUI_COMPILE_DEFINITIONS
    ${UNA_SDK_DEFINES_GUI_LVGL}
)
set(GUI_STACK_SIZE "24*1024")
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

una_app_build_app()
