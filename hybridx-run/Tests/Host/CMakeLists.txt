cmake_minimum_required(VERSION 3.21)
project(hybridx-run-host-tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ apart from the SDK's JSON reader and writer (over coreJSON),
# which the history file uses, as HybridX Streak's state file does. The SDK
# also provides the pinned GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

# The glance's pure parts (its service needs the kernel and is not here).
# GlanceLayout uses the SDK's GlanceControl.h constants, a plain C header.
set(GLANCE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Glance")
set(GLANCE_SOURCES
    "${GLANCE_LIBS}/Sources/GlanceView.cpp"
    "${GLANCE_LIBS}/Sources/GlanceLayout.cpp"
)

add_executable(hybridx-run-host-tests
    Vo2ProfileTest.cpp
    Vo2RunTest.cpp
    Vo2HistoryTest.cpp
    Vo2TextTest.cpp
    Vo2LogTest.cpp
    Vo2SettingsTest.cpp
    GlanceViewTest.cpp
    GlanceLayoutTest.cpp
    "${CORE_LIBS}/Sources/Vo2Profile.cpp"
    "${CORE_LIBS}/Sources/Vo2Run.cpp"
    "${CORE_LIBS}/Sources/Vo2History.cpp"
    "${CORE_LIBS}/Sources/Vo2Text.cpp"
    "${CORE_LIBS}/Sources/Vo2Log.cpp"
    "${CORE_LIBS}/Sources/Vo2Settings.cpp"
    ${GLANCE_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(hybridx-run-host-tests PRIVATE
    "${CORE_LIBS}/Header"
    "${GLANCE_LIBS}/Header"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
target_link_libraries(hybridx-run-host-tests PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME hybridx-run-host-tests COMMAND hybridx-run-host-tests)

# The glance's controls for a few sample histories, for
# docs/experiments/glance_preview.py to draw (the simulator cannot run glances).
add_executable(glance_preview
    tools/glance_preview.cpp
    "${CORE_LIBS}/Sources/Vo2Profile.cpp"
    "${CORE_LIBS}/Sources/Vo2History.cpp"
    "${CORE_LIBS}/Sources/Vo2Text.cpp"
    ${GLANCE_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(glance_preview PRIVATE
    "${CORE_LIBS}/Header"
    "${GLANCE_LIBS}/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
