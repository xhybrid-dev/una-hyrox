# HybridX Streak's glance -- a service-only app the kernel starts for the
# glances screen (Docs/service-lifecycle.md 3.3). Built like the SDK's
# GlanceSteps: APP_TYPE Glance, no icons, and no GUI_PATH, because
# una-app.cmake only builds a GUI when GUI_PATH or TOUCHGFX_PATH is defined.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

set(APP_NAME "HXStreakGlance")
set(APP_USER_NAME "Streak")
set(APP_FILE_NAME "HXStreakGlance")
set(APP_TYPE "Glance")
set(DEV_ID "HybridX")
set(APP_ID "D9923B245EF1C6FE")          # first 16 hex of md5("HybridXStreakGlance"); development ID
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
# Its own output folder, so the main app's Output/ keeps exactly one .uapp.
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output/Glance")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(APP_USE_ICONS Off)
file(MAKE_DIRECTORY "${OUTPUT_PATH}")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
include(${LIBS_PATH}/libs.cmake)

project(${APP_NAME})

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/streak-version.cmake)
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

set(SERVICE_SOURCES
    ${STREAK_CORE_SOURCES}
    ${STREAK_GLANCE_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${STREAK_GLANCE_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

una_app_build_app()
