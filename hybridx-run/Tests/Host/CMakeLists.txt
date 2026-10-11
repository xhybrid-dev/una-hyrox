cmake_minimum_required(VERSION 3.21)
project(hybridx-run-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ with no SDK headers; the SDK provides the pinned
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

add_executable(hybridx-run-host-tests
    Vo2ProfileTest.cpp
    Vo2RunTest.cpp
    Vo2HistoryTest.cpp
    Vo2TextTest.cpp
    "${CORE_LIBS}/Sources/Vo2Profile.cpp"
    "${CORE_LIBS}/Sources/Vo2Run.cpp"
    "${CORE_LIBS}/Sources/Vo2History.cpp"
    "${CORE_LIBS}/Sources/Vo2Text.cpp"
)
target_include_directories(hybridx-run-host-tests PRIVATE "${CORE_LIBS}/Header")
target_link_libraries(hybridx-run-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-run-host-tests COMMAND hybridx-run-host-tests)
