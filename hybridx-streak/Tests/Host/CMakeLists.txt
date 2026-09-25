cmake_minimum_required(VERSION 3.21)
project(hybridx-streak-host-tests C CXX)

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
file(GLOB STREAK_CORE_TEST_SOURCES CONFIGURE_DEPENDS "${STREAK_SOFTWARE}/Libs/Core/Sources/*.cpp")
# The SDK's FIT encoder and CRC: the reader checks CRCs with the SDK's own
# function, and the fixtures are written by the SDK's own writer.
set(SDK_FIT_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitCrc.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitWriter.cpp"
)
# The SDK's JSON reader and writer (over coreJSON), which the state file uses.
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

add_executable(hybridx-streak-host-tests
    WeekMathTest.cpp
    SummitsTest.cpp
    SummitGeometryTest.cpp
    CoachTest.cpp
    TreeFileSystemTest.cpp
    ProbeRunnerTest.cpp
    FitSessionReaderTest.cpp
    ActivityScannerTest.cpp
    StreakModelTest.cpp
    StateCodecTest.cpp
    GlanceLayoutTest.cpp
    "${STREAK_SOFTWARE}/Libs/Glance/Sources/GlanceLayout.cpp"
    support/TreeFileSystem.cpp
    support/FitFixture.cpp
    ${STREAK_CORE_TEST_SOURCES}
    ${SDK_FIT_SOURCES}
    ${SDK_JSON_SOURCES}
    "${STREAK_GUI}/src/summit/SummitGeometry.cpp"
    "${STREAK_GUI}/src/copy/Coach.cpp"
    "${STREAK_PROBE}/Sources/ProbeRunner.cpp"
)
target_include_directories(hybridx-streak-host-tests PRIVATE
    "${STREAK_SOFTWARE}/Libs/Core/Header"
    "${STREAK_GUI}/include"
    "${STREAK_PROBE}/Header"
    "${STREAK_SOFTWARE}/Libs/Glance/Header"
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
target_compile_definitions(hybridx-streak-host-tests PRIVATE
    RACE_FIT_DIR="${CMAKE_CURRENT_LIST_DIR}/../../../hybridx-race/docs/experiments/fit-candidates"
)
target_link_libraries(hybridx-streak-host-tests PRIVATE GTest::gtest_main)

# make_fit: writes fixture activities to disk for the simulator (sim_fixtures.sh).
add_executable(make_fit tools/make_fit.cpp support/FitFixture.cpp ${SDK_FIT_SOURCES})
target_include_directories(make_fit PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${STREAK_SOFTWARE}/Libs/Core/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
)

enable_testing()
add_test(NAME hybridx-streak-host-tests COMMAND hybridx-streak-host-tests)

# make_state: a saved history for the simulator (walkthrough, shield and summit flows).
add_executable(make_state tools/make_state.cpp ${STREAK_CORE_TEST_SOURCES} ${SDK_FIT_SOURCES} ${SDK_JSON_SOURCES})
target_include_directories(make_state PRIVATE
    "${STREAK_SOFTWARE}/Libs/Core/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)

# glance_preview: the glance's controls as text, for docs/experiments/glance_preview.py.
add_executable(glance_preview tools/glance_preview.cpp "${STREAK_SOFTWARE}/Libs/Glance/Sources/GlanceLayout.cpp")
target_include_directories(glance_preview PRIVATE
    "${STREAK_SOFTWARE}/Libs/Glance/Header"
    "${STREAK_SOFTWARE}/Libs/Core/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
)
