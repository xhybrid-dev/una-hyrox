cmake_minimum_required(VERSION 3.21)
project(hybridx-link-host-tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The link is plain C++ apart from the SDK's JSON reader and writer (over
# coreJSON), as Run's history file is. The SDK also provides the pinned
# GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(LINK_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Link")
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

add_executable(hybridx-link-host-tests
    ActivityLinkTest.cpp
    StreakNowTest.cpp
    "${LINK_LIBS}/Sources/ActivityLink.cpp"
    "${LINK_LIBS}/Sources/StreakNow.cpp"
    ${SDK_JSON_SOURCES}
)
target_include_directories(hybridx-link-host-tests PRIVATE
    "${LINK_LIBS}/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
target_link_libraries(hybridx-link-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-link-host-tests COMMAND hybridx-link-host-tests)
