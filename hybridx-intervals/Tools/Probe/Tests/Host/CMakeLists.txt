cmake_minimum_required(VERSION 3.21)
project(hybridx-intervals-probe-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The SDK provides the pinned GoogleTest fetch and the IFileSystem interface
# ProbeRunner is tested against (via FlatFileSystem, support/).
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(PROBE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs")

add_executable(hybridx-intervals-probe-host-tests
    ProbeRunnerTest.cpp
    support/FlatFileSystem.cpp
    "${PROBE_LIBS}/Sources/ProbeRunner.cpp"
)
target_include_directories(hybridx-intervals-probe-host-tests PRIVATE
    "${PROBE_LIBS}/Header"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UNA_SDK_ROOT}/Libs/Header"
)
target_link_libraries(hybridx-intervals-probe-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-intervals-probe-host-tests COMMAND hybridx-intervals-probe-host-tests)
