cmake_minimum_required(VERSION 3.21)
project(hybridx-training-host-tests C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core is plain C++ apart from three things from the SDK: the workout program
# struct it builds (SDK::Workout::Program), the JSON reader and writer (the plan
# file), and, in the tests only, the SDK's FIT workout writer and reader, its
# workout engine and its step labels, to prove a built program is one the watch
# can record and run. The SDK also provides the pinned GoogleTest fetch.
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR
        "UNA_SDK is not set. Point it at the una-sdk checkout, e.g.\n"
        "  export UNA_SDK=/path/to/workspace/una-sdk"
    )
endif()
set(UNA_SDK_ROOT "$ENV{UNA_SDK}")

include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CORE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Core")
set(TOOLS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Tools")

set(CORE_SOURCES
    "${CORE_LIBS}/Sources/PaceModel.cpp"
    "${CORE_LIBS}/Sources/WeekLayout.cpp"
    "${CORE_LIBS}/Sources/PlanCalendar.cpp"
    "${CORE_LIBS}/Sources/PlanBuilder.cpp"
    "${CORE_LIBS}/Sources/PlanState.cpp"
    "${CORE_LIBS}/Sources/PlanSchedule.cpp"
    "${CORE_LIBS}/Sources/PlanProgress.cpp"
)
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)
set(SDK_WORKOUT_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/Workout/WorkoutEngine.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Workout/StepLabel.cpp"
)
set(SDK_FIT_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitCrc.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitWriter.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitWorkoutReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitWorkoutWriter.cpp"
)
set(CORE_INCLUDES
    "${CORE_LIBS}/Header"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
)

add_executable(hybridx-training-host-tests
    DateTest.cpp
    TimeTextTest.cpp
    PaceModelTest.cpp
    PlanDataTest.cpp
    WeekLayoutTest.cpp
    PlanCalendarTest.cpp
    PlanBuilderTest.cpp
    PlanStateTest.cpp
    PlanTextTest.cpp
    PlanScheduleTest.cpp
    PlanProgressTest.cpp
    RepJudgeTest.cpp
    HoldTimerTest.cpp
    DanielsTest.cpp
    GoldenTest.cpp
    "${TOOLS_DIR}/plan_print/PlanPrint.cpp"
    ${CORE_SOURCES}
    ${SDK_JSON_SOURCES}
    ${SDK_WORKOUT_SOURCES}
    ${SDK_FIT_SOURCES}
)
target_include_directories(hybridx-training-host-tests PRIVATE
    ${CORE_INCLUDES}
    "${TOOLS_DIR}/plan_print"
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UNA_SDK_ROOT}/Tests/Host/support"
)
target_compile_definitions(hybridx-training-host-tests PRIVATE
    XTRAINING_GOLDEN_DIR="${CMAKE_CURRENT_LIST_DIR}/golden"
    XTRAINING_FIXTURE_DIR="${CMAKE_CURRENT_LIST_DIR}/fixtures"
)
target_link_libraries(hybridx-training-host-tests PRIVATE GTest::gtest_main)

# plan_print: a whole plan for any runner, as Markdown (TP1's gate). The same
# code writes the golden files the tests compare against.
add_executable(plan_print
    "${TOOLS_DIR}/plan_print/plan_print.cpp"
    "${TOOLS_DIR}/plan_print/PlanPrint.cpp"
    ${CORE_SOURCES}
    ${SDK_JSON_SOURCES}
    ${SDK_WORKOUT_SOURCES}
)
target_include_directories(plan_print PRIVATE ${CORE_INCLUDES} "${TOOLS_DIR}/plan_print")

enable_testing()
add_test(NAME hybridx-training-host-tests COMMAND hybridx-training-host-tests)
# The generated plan header must match the plan files.
add_test(NAME plan-data-current
         COMMAND "${Python3_EXECUTABLE}" "${TOOLS_DIR}/Plans/gen_plans.py" --check)
