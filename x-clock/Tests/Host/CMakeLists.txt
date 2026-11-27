cmake_minimum_required(VERSION 3.21)
project(x-clock-host-tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The Clock Probe's core is plain C++ with no kernel (x-clock/docs/PROBE.md). The SDK provides the
# pinned GoogleTest fetch, the IFileSystem interface and the JSON reader the copied readers use.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

set(PROBE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Tools/Probe/Software/Libs")

file(GLOB PROBE_CORE_SOURCES     CONFIGURE_DEPENDS "${PROBE_LIBS}/Core/Sources/*.cpp")
file(GLOB PROBE_LINK_SOURCES     CONFIGURE_DEPENDS "${PROBE_LIBS}/Link/Sources/*.cpp")
file(GLOB PROBE_TRAINING_SOURCES CONFIGURE_DEPENDS "${PROBE_LIBS}/Training/Sources/*.cpp")
file(GLOB PROBE_VO2_SOURCES      CONFIGURE_DEPENDS "${PROBE_LIBS}/Vo2/Sources/*.cpp")

set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

add_executable(x-clock-host-tests
    KnownFilesTest.cpp
    FileProbeTest.cpp
    ScanTest.cpp
    WeekViewTest.cpp
    LogsTest.cpp
    ProbeTextTest.cpp
    ProbeStateTest.cpp
    support/TreeFileSystem.cpp
    ${PROBE_CORE_SOURCES}
    ${PROBE_LINK_SOURCES}
    ${PROBE_TRAINING_SOURCES}
    ${PROBE_VO2_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(x-clock-host-tests PRIVATE
    "${PROBE_LIBS}/Core/Header"
    "${PROBE_LIBS}/Link/Header"
    "${PROBE_LIBS}/Training/Header"
    "${PROBE_LIBS}/Vo2/Header"
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)
target_link_libraries(x-clock-host-tests PRIVATE GTest::gtest_main)

# A pretend watch's shared folder for the simulator, in the real format (docs/experiments/sim_fixtures.sh).
add_executable(make_family
    tools/make_family.cpp
    support/TreeFileSystem.cpp
    ${PROBE_CORE_SOURCES}
    ${PROBE_LINK_SOURCES}
    ${PROBE_TRAINING_SOURCES}
    ${PROBE_VO2_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(make_family PRIVATE
    "${PROBE_LIBS}/Core/Header"
    "${PROBE_LIBS}/Link/Header"
    "${PROBE_LIBS}/Training/Header"
    "${PROBE_LIBS}/Vo2/Header"
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)

# The widest text each screen row can show, for docs/experiments/audit.py.
add_executable(worst_rows
    tools/worst_rows.cpp
    support/TreeFileSystem.cpp
    ${PROBE_CORE_SOURCES}
    ${PROBE_LINK_SOURCES}
    ${PROBE_TRAINING_SOURCES}
    ${PROBE_VO2_SOURCES}
    ${SDK_JSON_SOURCES}
)
target_include_directories(worst_rows PRIVATE
    "${PROBE_LIBS}/Core/Header"
    "${PROBE_LIBS}/Link/Header"
    "${PROBE_LIBS}/Training/Header"
    "${PROBE_LIBS}/Vo2/Header"
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)

enable_testing()
add_test(NAME x-clock-host-tests COMMAND x-clock-host-tests)
