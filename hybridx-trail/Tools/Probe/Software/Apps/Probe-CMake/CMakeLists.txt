# Trail Probe -- the throwaway Phase T0 app that answers Gate T0: does a GPX
# copied over USB into this app's own Routes/ folder reach the app, can it be
# read into a route, what memory is there to hold one, and what do GPS and the
# compass give on a real watch? (hybridx-trail PLAN T0; how to run it:
# hybridx-trail/docs/PROBE.md)
#
# Built like HybridX Race/Streak, with its own name and ID so it installs
# standalone. Its version is fixed: it is never released, so it takes no tag.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "HXTrailProbe")
set(APP_USER_NAME "Trail Probe")
set(APP_FILE_NAME "HXTrailProbe")
set(APP_TYPE "Utility")
set(DEV_ID "HybridX")
set(APP_ID "0BFC65580EBE7A2B")          # first 16 hex of md5("HXTrailProbe"); development ID
set(BUILD_VERSION "0.1.0")
set(PROBE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(PROBE_LIBS "${PROBE_ROOT}/Software/Libs")
set(CORE_LIBS "${PROBE_ROOT}/../../Software/Libs/Core")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Probe-GUI")
# app_merging.py requires -normal_icon/-small_icon for every non-Glance app
# type (hybridx-intervals NOTES), so placeholder artwork: Resources/make_icons.py.
set(RESOURCES_PATH "${PROBE_ROOT}/../../Resources")
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)

project(${APP_NAME})
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Service: the route check (the real app's GPX core), memory and sensors.
set(SERVICE_SOURCES
    ${CORE_LIBS}/Sources/GeoPoint.cpp
    ${CORE_LIBS}/Sources/GpxReader.cpp
    ${CORE_LIBS}/Sources/RouteBuilder.cpp
    ${CORE_LIBS}/Sources/RouteMath.cpp
    ${PROBE_LIBS}/Sources/ProbeRunner.cpp
    ${PROBE_LIBS}/Sources/Service.cpp
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${PROBE_LIBS}/Header
    ${CORE_LIBS}/Header
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

# GUI: one screen, three Poppins faces (assets/fonts, this probe's own copy).
set(GUI_SOURCES
    ${GUI_PATH}/gui/src/ProbeGui.cpp
    ${GUI_PATH}/assets/fonts/poppins_semibold_25.c
    ${GUI_PATH}/assets/fonts/poppins_regular_16.c
    ${GUI_PATH}/assets/fonts/poppins_regular_14.c
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI_LVGL}
)
set(GUI_INCLUDE_DIRS
    ${GUI_PATH}/gui/include
    ${PROBE_LIBS}/Header
    ${CORE_LIBS}/Header
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
