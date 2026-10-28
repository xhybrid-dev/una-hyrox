# Host tests for hybridx-ride: the bike probe's lines for now.
#   cmake -S hybridx-ride/Tests/Host -B hybridx-ride/build-tests
#   cmake --build hybridx-ride/build-tests && hybridx-ride/build-tests/hybridx-ride-host-tests
cmake_minimum_required(VERSION 3.21)
project(hybridx-ride-host-tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()
include("$ENV{UNA_SDK}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(PROBE_CORE "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs/Core")
add_executable(hybridx-ride-host-tests ProbeTextTest.cpp)
target_include_directories(hybridx-ride-host-tests PRIVATE "${PROBE_CORE}/Header")
target_compile_options(hybridx-ride-host-tests PRIVATE -Wall -Wextra)
target_link_libraries(hybridx-ride-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-ride-host-tests COMMAND hybridx-ride-host-tests)
