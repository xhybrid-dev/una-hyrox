cmake_minimum_required(VERSION 3.21)
project(x-weighin-host-tests CXX)

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

add_executable(x-weighin-host-tests
    WeighProfileTest.cpp
    WeighTextTest.cpp
    "${CORE_LIBS}/Sources/WeighProfile.cpp"
)
target_include_directories(x-weighin-host-tests PRIVATE "${CORE_LIBS}/Header")
target_link_libraries(x-weighin-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME x-weighin-host-tests COMMAND x-weighin-host-tests)
