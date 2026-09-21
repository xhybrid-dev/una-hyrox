cmake_minimum_required(VERSION 3.21)
project(hybridx-race-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The SDK is only borrowed for its pinned GoogleTest fetch. RaceModel itself has
# no SDK dependency at all (brief 7.4), which is what keeps this suite fast and
# buildable without the ARM toolchain.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(APP_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs")

add_executable(hybridx-race-host-tests
    RaceTemplateTest.cpp
    RaceStateTest.cpp
    RaceTimingTest.cpp
    LapAccumulatorTest.cpp
    "${APP_LIBS}/Sources/RaceModel.cpp"
)

target_include_directories(hybridx-race-host-tests PRIVATE "${APP_LIBS}/Header")
target_link_libraries(hybridx-race-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-race-host-tests COMMAND hybridx-race-host-tests)
