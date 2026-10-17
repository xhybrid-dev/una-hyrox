cmake_minimum_required(VERSION 3.21)
project(x-trailquest-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ with no kernel, so it builds and tests on the host. The SDK
# provides the pinned GoogleTest fetch (as for the other apps).
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "${CORE_LIBS}/Sources/*.cpp")
set(CORE_INCLUDES
    "${CORE_LIBS}/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*Test.cpp")

add_executable(x-trailquest-host-tests
    ${TEST_SOURCES}
    ${CORE_SOURCES}
)
target_include_directories(x-trailquest-host-tests PRIVATE
    ${CORE_INCLUDES}
    "${CMAKE_CURRENT_LIST_DIR}"
)
target_link_libraries(x-trailquest-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME x-trailquest-host-tests COMMAND x-trailquest-host-tests)
