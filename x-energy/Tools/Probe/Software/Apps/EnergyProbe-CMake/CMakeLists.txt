# The Energy Probe (x-energy/docs/PROBE.md, Gate SL0): a resident logger that records every
# sensor a sleep and energy app could use, a row a minute, with nights set to record or not so
# the battery drain can be compared. A throwaway tool, not an app for the store. Started from
# the Bike Probe's CMakeLists.txt (hybridx-ride/Tools/Probe); the GUI is LVGL.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "EnergyProbe")
set(APP_USER_NAME "Energy Probe")
set(APP_FILE_NAME "EnergyProbe")
set(APP_TYPE "Utility")
# Started at boot and again after USB mode, with no screen (Docs/service-lifecycle.md 3.2),
# as the SDK's Alarm example: the logger must run all night with nobody looking.
set(APP_AUTOSTART On)
set(DEV_ID "HybridX")
set(APP_ID "939CAA3975888E07")          # development ID: first 16 hex of md5("EnergyProbe")
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LVGL-GUI")
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
include(${LIBS_PATH}/libs.cmake)
include(${GUI_PATH}/lvgl-gui.cmake)

project(${APP_NAME})

# A probe has no release tags: a fixed version.
set(BUILD_VERSION "0.1.0")

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
set(GUI_STACK_SIZE "24*1024")
set(GUI_RAM_LENGTH "900K")
una_app_build_gui(${APP_NAME}GUI.elf)

una_app_build_app()
