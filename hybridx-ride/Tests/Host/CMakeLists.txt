cmake_minimum_required(VERSION 3.21)
project(hybridx-ride-host-tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ with no kernel, apart from the settings file, which uses the
# SDK's JSON reader and writer (over coreJSON). The SDK provides the pinned
# GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")
# The Gate B0 probe's text helpers keep their tests here too.
set(PROBE_CORE "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs/Core")

# Anything the GUI uses is header-only (the GUI process links none of the
# library sources); the settings file is the service's.
set(CORE_SOURCES
    "${CORE_LIBS}/Sources/RideSettings.cpp"
)
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

add_executable(hybridx-ride-host-tests
    RideModelTest.cpp
    RideTextTest.cpp
    RideFitTest.cpp
    RideSettingsTest.cpp
    RideMotionTest.cpp
    LapTimingTest.cpp
    HoldTimerTest.cpp
    SecondGateTest.cpp
    LapLockoutTest.cpp
    ProbeTextTest.cpp
    ${CORE_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(hybridx-ride-host-tests PRIVATE
    "${CORE_LIBS}/Header"
    "${PROBE_CORE}/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
target_link_libraries(hybridx-ride-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-ride-host-tests COMMAND hybridx-ride-host-tests)
