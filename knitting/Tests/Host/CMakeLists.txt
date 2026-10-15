cmake_minimum_required(VERSION 3.21)
project(knitting-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ with no kernel. The SDK provides the pinned GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")

add_executable(knitting-host-tests
    RowCounterTest.cpp
)
target_include_directories(knitting-host-tests PRIVATE "${CORE_LIBS}/Header")
target_link_libraries(knitting-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME knitting-host-tests COMMAND knitting-host-tests)
