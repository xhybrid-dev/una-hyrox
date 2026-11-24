cmake_minimum_required(VERSION 3.21)
project(x-energy-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The Energy Probe's core is plain C++ with no kernel (x-energy/docs/PROBE.md). The SDK
# provides the pinned GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(PROBE_CORE "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs/Core")

add_executable(x-energy-host-tests
    ScheduleTest.cpp
    MinuteEpochTest.cpp
    NightSummaryTest.cpp
    StateFileTest.cpp
    FixedTest.cpp
    "${PROBE_CORE}/Sources/Schedule.cpp"
    "${PROBE_CORE}/Sources/MinuteEpoch.cpp"
    "${PROBE_CORE}/Sources/NightSummary.cpp"
    "${PROBE_CORE}/Sources/StateFile.cpp"
)
target_include_directories(x-energy-host-tests PRIVATE "${PROBE_CORE}/Header")
target_link_libraries(x-energy-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME x-energy-host-tests COMMAND x-energy-host-tests)
