# The Clock Probe (x-clock/docs/PROBE.md, Gate F0): a Clockface-type app that finds out what a
# watch face can do on the watch: read the family's files in ../SharedData, hear a wrist raise,
# be ticked while the screen is dark, and write its own log. A throwaway tool, not an app for the
# store. Started from the Energy Probe's CMakeLists.txt (x-energy/Tools/Probe); the GUI is LVGL.
#
# APP_TYPE Clockface is what makes the kernel offer it in Settings > Watch Face (Docs/writing-a-clockface.md).
# Not autostarted: the kernel gives a face the display and starts it with its GUI.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "ClockProbe")
set(APP_USER_NAME "Clock Probe")
set(APP_FILE_NAME "ClockProbe")
set(APP_TYPE "Clockface")
set(DEV_ID "HybridX")
set(APP_ID "D3D9144125A884DD")          # development ID: first 16 hex of md5("ClockProbe")
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
