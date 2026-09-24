cmake_minimum_required(VERSION 3.21)
project(hybridx-streak-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The SDK provides the pinned GoogleTest fetch and, for the probe's tests, the
# IFileSystem interface. The code under test is pure C++ (Core) or pure C++
# that happens to live in the GUI tree (the summit geometry and the coach's
# copy), compiled here directly.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(STREAK_SOFTWARE "${CMAKE_CURRENT_LIST_DIR}/../../Software")
set(STREAK_GUI      "${STREAK_SOFTWARE}/Apps/LVGL-GUI/gui")
set(STREAK_PROBE    "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs")

add_executable(hybridx-streak-host-tests
    WeekMathTest.cpp
    SummitsTest.cpp
    SummitGeometryTest.cpp
    CoachTest.cpp
    TreeFileSystemTest.cpp
    ProbeRunnerTest.cpp
    support/TreeFileSystem.cpp
    "${STREAK_GUI}/src/summit/SummitGeometry.cpp"
    "${STREAK_GUI}/src/copy/Coach.cpp"
    "${STREAK_PROBE}/Sources/ProbeRunner.cpp"
)
target_include_directories(hybridx-streak-host-tests PRIVATE
    "${STREAK_SOFTWARE}/Libs/Core/Header"
    "${STREAK_GUI}/include"
    "${STREAK_PROBE}/Header"
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
)
target_link_libraries(hybridx-streak-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-streak-host-tests COMMAND hybridx-streak-host-tests)
