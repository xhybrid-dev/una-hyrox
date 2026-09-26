cmake_minimum_required(VERSION 3.21)
project(hybridx-trail-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The SDK provides the pinned GoogleTest fetch and the IFileSystem interface
# the probe's Runner is tested against (via FlatFileSystem, support/).
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")
set(PROBE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs")

add_executable(hybridx-trail-host-tests
    GeoPointTest.cpp
    GpxReaderTest.cpp
    RouteBuilderTest.cpp
    RouteMathTest.cpp
    ProbeRunnerTest.cpp
    support/FlatFileSystem.cpp
    "${CORE_LIBS}/Sources/GeoPoint.cpp"
    "${CORE_LIBS}/Sources/GpxReader.cpp"
    "${CORE_LIBS}/Sources/RouteBuilder.cpp"
    "${CORE_LIBS}/Sources/RouteMath.cpp"
    "${PROBE_LIBS}/Sources/ProbeRunner.cpp"
)
target_include_directories(hybridx-trail-host-tests PRIVATE
    "${CORE_LIBS}/Header"
    "${PROBE_LIBS}/Header"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UNA_SDK_ROOT}/Libs/Header"
)
target_compile_definitions(hybridx-trail-host-tests PRIVATE
    TRAIL_FIXTURES="${CMAKE_CURRENT_LIST_DIR}/fixtures"
)
target_link_libraries(hybridx-trail-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-trail-host-tests COMMAND hybridx-trail-host-tests)
