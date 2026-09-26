# Trail Probe PC simulator: the probe's service and GUI against the SDK's
# mock kernel, for checking the probe itself before it goes on a watch.
#
# The mock file system's root is "../../../../../Output/" from the working
# directory (una-sdk Simulator/Kernel.cpp), and that's plain host-directory
# traversal, so this simulator proves the reading side of T0 (can the app find
# and read a GPX dropped into its own Routes/ folder). It says nothing about
# USB copying, real GPS or the compass (the simulator has no magnetometer), or
# the watch's memory -- that's what Jon's run on real hardware is for
# (docs/PROBE.md).
#
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build
cmake_minimum_required(VERSION 3.21)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

project(TrailProbeSimulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include($ENV{UNA_SDK}/cmake/una-simulator.cmake)

set(PROBE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
set(PROBE_LIBS "${PROBE_ROOT}/Software/Libs")
set(CORE_LIBS "${PROBE_ROOT}/../../Software/Libs/Core")

set(SIM_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SIM_BIN_DIR}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${SIM_BIN_DIR}")
endforeach()

add_executable(TrailProbeSimulator
    main.cpp
    ../gui/src/ProbeGui.cpp
    ../assets/fonts/poppins_semibold_25.c
    ../assets/fonts/poppins_regular_16.c
    ../assets/fonts/poppins_regular_14.c
    ${CORE_LIBS}/Sources/GeoPoint.cpp
    ${CORE_LIBS}/Sources/GpxReader.cpp
    ${CORE_LIBS}/Sources/RouteBuilder.cpp
    ${CORE_LIBS}/Sources/RouteMath.cpp
    ${PROBE_LIBS}/Sources/ProbeRunner.cpp
    ${PROBE_LIBS}/Sources/Service.cpp
    ${UNA_SDK_SOURCES_SIMULATOR}
    ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
    ${UNA_SDK_SOURCES_SIMULATOR_LVGL}
)

target_include_directories(TrailProbeSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/include   # also how the SDK finds ConfigurationSimulator.hpp
    ${PROBE_LIBS}/Header
    ${CORE_LIBS}/Header
    ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)

target_compile_definitions(TrailProbeSimulator PRIVATE
    ${UNA_SDK_DEFINES_SIMULATOR}
    ${UNA_SDK_DEFINES_GUI_LVGL}
    APP_ID="0BFC65580EBE7A2B"
    APP_NAME="HXTrailProbe"
    DEV_ID="HybridX Simulator"
    BUILD_VERSION="0.1.0-sim"
)

find_package(Threads REQUIRED)
target_link_libraries(TrailProbeSimulator PRIVATE Threads::Threads)
una_simulator_link_sdl2(TrailProbeSimulator)

if(MSVC)
    target_compile_options(TrailProbeSimulator PRIVATE /W3 /MP)
else()
    target_compile_options(TrailProbeSimulator PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
