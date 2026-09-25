# HybridX Streak PC simulator: the app's service and LVGL GUI processes running
# against the SDK's mock kernel, with the display in an SDL2 window.
# Laid out as hybridx-race's simulator.
#
# Linux (needs libsdl2-dev):
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
# Run from build/bin so the mock file system lands in ../../../../../Output,
# i.e. Software/Output.
cmake_minimum_required(VERSION 3.21)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

project(HybridXStreakSimulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include($ENV{UNA_SDK}/cmake/una-simulator.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../Libs/libs.cmake)   # STREAK_CORE_*, STREAK_APP_*
include(${CMAKE_CURRENT_SOURCE_DIR}/../lvgl-gui.cmake)          # GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS, STREAK_GUI_DEFINES

set(SIM_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SIM_BIN_DIR}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${SIM_BIN_DIR}")
endforeach()

add_executable(HybridXStreakSimulator
    main.cpp
    ${GUI_APP_SOURCES}
    ${STREAK_CORE_SOURCES}
    ${STREAK_APP_SOURCES}
    ${UNA_SDK_SOURCES_SIMULATOR}
    ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
    ${UNA_SDK_SOURCES_SIMULATOR_LVGL}
)

target_include_directories(HybridXStreakSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # ConfigurationSimulator.hpp
    ${GUI_APP_INCLUDE_DIRS}
    ${STREAK_APP_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)

target_compile_definitions(HybridXStreakSimulator PRIVATE
    ${UNA_SDK_DEFINES_SIMULATOR}
    ${UNA_SDK_DEFINES_GUI_LVGL}
    ${STREAK_GUI_DEFINES}
    APP_ID="A75434BA114DFEBE"
    APP_NAME="HybridXStreak"
    DEV_ID="HybridX Simulator"
    BUILD_VERSION="1.0.0-sim"
)

find_package(Threads REQUIRED)
target_link_libraries(HybridXStreakSimulator PRIVATE Threads::Threads)
una_simulator_link_sdl2(HybridXStreakSimulator)

if(MSVC)
    target_compile_options(HybridXStreakSimulator PRIVATE /W3 /MP)
else()
    target_compile_options(HybridXStreakSimulator PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
