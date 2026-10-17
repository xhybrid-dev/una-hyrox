# X Weigh-in's glance -- a service-only app the kernel starts for the glances
# screen (Docs/service-lifecycle.md 3.3). See ../../../docs/NOTES.md, W3.
#
# Built like HybridX Streak's glance and the SDK's GlanceSteps: APP_TYPE Glance,
# no icons, and no GUI_PATH, because una-app.cmake only builds a GUI when
# GUI_PATH or TOUCHGFX_PATH is defined. It reads the app's SharedData copy of
# the log and never writes.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "XWeighInGlance")
set(APP_USER_NAME "X Weigh-in")         # the label on the glances screen
set(APP_FILE_NAME "XWeighInGlance")
set(APP_TYPE "Glance")
set(DEV_ID "HybridX")
set(APP_ID "63F628D5251835AD")          # issued by the UNA portal (13 Oct 2026); the development ID was 5543E86906F4A1B3
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
# Its own output folder, so the app's Output/ keeps exactly one .uapp.
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output/Glance")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(APP_USE_ICONS Off)
# Output/ is git-ignored; the linker needs it to exist for its .map files.
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
# Includes WEIGHIN_CORE_SOURCES, WEIGHIN_GLANCE_SOURCES, WEIGHIN_GLANCE_INCLUDE_DIRS
include(${LIBS_PATH}/libs.cmake)

project(${APP_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/weighin-version.cmake)
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

set(SERVICE_SOURCES
    ${WEIGHIN_CORE_SOURCES}
    ${WEIGHIN_GLANCE_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${WEIGHIN_GLANCE_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

una_app_build_app()
