cmake_minimum_required(VERSION 3.21)
project(trailquest-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ with no kernel, so it builds and tests on the host. The save
# store sees the SDK's IFileSystem interface (a header) and is tested on an
# in-memory tree (support/TreeFileSystem, from Knitting, itself from Streak).
# The SDK provides the pinned GoogleTest fetch (as for the other apps).
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

add_executable(trailquest-host-tests
    ${TEST_SOURCES}
    ${CORE_SOURCES}
    support/TreeFileSystem.cpp
)
target_include_directories(trailquest-host-tests PRIVATE
    ${CORE_INCLUDES}
    "${CMAKE_CURRENT_LIST_DIR}"
)
target_link_libraries(trailquest-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME trailquest-host-tests COMMAND trailquest-host-tests)

# Tools: the balance simulator (every spirit at par, many seeded duels) and the
# run simulator (whole games), with the simulated player in tools/Policy.hpp.
# They write docs/BALANCE.md (see its header for the commands).
foreach(tool balance_sim run_sim)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/tools/${tool}.cpp")
        add_executable(${tool} tools/${tool}.cpp ${CORE_SOURCES})
        target_include_directories(${tool} PRIVATE ${CORE_INCLUDES} "${CMAKE_CURRENT_LIST_DIR}/tools")
    endif()
endforeach()
