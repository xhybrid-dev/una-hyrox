cmake_minimum_required(VERSION 3.21)
project(hybridx-intervals-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is SDK-free (no una-sdk headers anywhere in Software/Libs/Core), but
# the SDK still provides the pinned GoogleTest fetch used across this repo.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")

add_executable(hybridx-intervals-host-tests
    WorkoutTypesTest.cpp
    WorkoutValidationTest.cpp
    HrZonesTest.cpp
    TargetEvaluatorTest.cpp
    WorkoutEngineTest.cpp
    WorkoutEventsTest.cpp
    "${CORE_LIBS}/Sources/WorkoutValidation.cpp"
    "${CORE_LIBS}/Sources/HrZones.cpp"
    "${CORE_LIBS}/Sources/TargetEvaluator.cpp"
    "${CORE_LIBS}/Sources/WorkoutEngine.cpp"
)
target_include_directories(hybridx-intervals-host-tests PRIVATE
    "${CORE_LIBS}/Header"
    "${CMAKE_CURRENT_LIST_DIR}"
)
target_link_libraries(hybridx-intervals-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-intervals-host-tests COMMAND hybridx-intervals-host-tests)
