# Streak Probe PC simulator: the probe's service and GUI against the SDK's
# mock kernel, for checking the probe itself before it goes on a watch.
#
# The mock file system's root is "../../../../../Output/" from the working
# directory (una-sdk Simulator/Kernel.cpp), and ".." from there is plain
# host-directory traversal, so the simulator says nothing about what the
# watch allows. Run it from a scratch tree seeded with a fake app's
# Activity/ folder to see every check work end to end (docs/PROBE.md).
#
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build
cmake_minimum_required(VERSION 3.21)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

project(StreakProbeSimulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include($ENV{UNA_SDK}/cmake/una-simulator.cmake)

set(PROBE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/../../../Libs")
set(STREAK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..")
set(STREAK_FONTS "${STREAK_ROOT}/Software/Apps/LVGL-GUI/assets/fonts")

set(SIM_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SIM_BIN_DIR}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${SIM_BIN_DIR}")
endforeach()

add_executable(StreakProbeSimulator
    main.cpp
    ../gui/src/ProbeGui.cpp
    ${STREAK_FONTS}/poppins_semibold_25.c
    ${STREAK_FONTS}/poppins_regular_16.c
    ${STREAK_FONTS}/poppins_regular_14.c
    ${PROBE_LIBS}/Sources/ProbeRunner.cpp
    ${PROBE_LIBS}/Sources/Service.cpp
    ${UNA_SDK_SOURCES_SIMULATOR}
    ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
    ${UNA_SDK_SOURCES_SIMULATOR_LVGL}
)

target_include_directories(StreakProbeSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/include   # also how the SDK finds ConfigurationSimulator.hpp
    ${PROBE_LIBS}/Header
    ${STREAK_ROOT}/Software/Libs/Core/Header
    ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)

target_compile_definitions(StreakProbeSimulator PRIVATE
    ${UNA_SDK_DEFINES_SIMULATOR}
    ${UNA_SDK_DEFINES_GUI_LVGL}
    APP_ID="DE9CF1F8FFF3776D"
    APP_NAME="HXStreakProbe"
    DEV_ID="HybridX Simulator"
    BUILD_VERSION="0.1.0-sim"
)

find_package(Threads REQUIRED)
target_link_libraries(StreakProbeSimulator PRIVATE Threads::Threads)
una_simulator_link_sdl2(StreakProbeSimulator)

if(MSVC)
    target_compile_options(StreakProbeSimulator PRIVATE /W3 /MP)
else()
    target_compile_options(StreakProbeSimulator PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
