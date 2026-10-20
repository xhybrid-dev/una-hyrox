cmake_minimum_required(VERSION 3.21)
project(x-sauna-host-tests CXX)

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

set(CORE_SOURCES
    "${CORE_LIBS}/Sources/ProbeCsv.cpp"
)

add_executable(x-sauna-host-tests
    FixedPointTest.cpp
    ProbeStatsTest.cpp
    ProbeCsvTest.cpp
    SimTempTest.cpp
    ${CORE_SOURCES}
)
target_include_directories(x-sauna-host-tests PRIVATE
    "${CORE_LIBS}/Header"
)
target_link_libraries(x-sauna-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME x-sauna-host-tests COMMAND x-sauna-host-tests)
