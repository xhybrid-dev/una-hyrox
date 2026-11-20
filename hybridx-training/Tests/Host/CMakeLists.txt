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
    "${CORE_LIBS}/Sources/RunVo2Max.cpp"
    "${CORE_LIBS}/Sources/PlanRules.cpp"
    "${CORE_LIBS}/Sources/TodayFile.cpp"
    "${CORE_LIBS}/Sources/TodayBuild.cpp"
    "${CORE_LIBS}/Sources/SessionReview.cpp"
    "${CORE_LIBS}/Sources/ReviewLog.cpp"
    "${CORE_LIBS}/Sources/ReviewInsight.cpp"
)
# The glance's pure parts (its service needs the kernel and is not here). GlanceLayout uses the
# SDK's GlanceControl.h constants, a plain C header, which CORE_INCLUDES reach.
set(GLANCE_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs/Glance")
set(GLANCE_SOURCES
    "${GLANCE_LIBS}/Sources/GlanceView.cpp"
    "${GLANCE_LIBS}/Sources/GlanceLayout.cpp"
)
set(SDK_JSON_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)
set(SDK_WORKOUT_SOURCES
    "${UNA_SDK_ROOT}/Libs/Source/Workout/WorkoutAlerts.cpp"
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
    ZoneDebounceTest.cpp
    PlanStateTest.cpp
    PlanTextTest.cpp
    PlanScheduleTest.cpp
    PlanProgressTest.cpp
    RepJudgeTest.cpp
    HoldTimerTest.cpp
    DanielsTest.cpp
    RunVo2MaxTest.cpp
    PlanRulesTest.cpp
    TodayFileTest.cpp
    GlanceViewTest.cpp
    GlanceLayoutTest.cpp
    SessionReviewTest.cpp
    ReviewLogTest.cpp
    ReviewInsightTest.cpp
    GoldenTest.cpp
    "${TOOLS_DIR}/plan_print/PlanPrint.cpp"
    "${TOOLS_DIR}/plan_sim/PlanSim.cpp"
    ${CORE_SOURCES}
    ${GLANCE_SOURCES}
    ${SDK_JSON_SOURCES}
    ${SDK_WORKOUT_SOURCES}
    ${SDK_FIT_SOURCES}
)
target_include_directories(hybridx-training-host-tests PRIVATE
    ${CORE_INCLUDES}
    "${TOOLS_DIR}/plan_print"
    "${TOOLS_DIR}/plan_sim"
    "${GLANCE_LIBS}/Header"
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

# plan_sim: the plan's memory as scenarios (TP5's gate). The golden test uses the same code.
add_executable(plan_sim
    "${TOOLS_DIR}/plan_sim/plan_sim.cpp"
    "${TOOLS_DIR}/plan_sim/PlanSim.cpp"
    ${CORE_SOURCES}
    ${SDK_JSON_SOURCES}
    ${SDK_WORKOUT_SOURCES}
)
target_include_directories(plan_sim PRIVATE ${CORE_INCLUDES} "${TOOLS_DIR}/plan_sim")

# glance_preview: the glance's controls for sample plans, for docs/experiments/glance_preview.py
# to draw (the simulator cannot run glances).
add_executable(glance_preview
    tools/glance_preview.cpp
    ${CORE_SOURCES}
    ${GLANCE_SOURCES}
    ${SDK_JSON_SOURCES}
    ${SDK_WORKOUT_SOURCES}
)
target_include_directories(glance_preview PRIVATE ${CORE_INCLUDES} "${GLANCE_LIBS}/Header")

# today_print: a training_today.json read with the core's reader, and what the glance shows (TP6).
add_executable(today_print
    "${TOOLS_DIR}/today_print/today_print.cpp"
    "${CORE_LIBS}/Sources/TodayFile.cpp"
    ${SDK_JSON_SOURCES}
)
target_include_directories(today_print PRIVATE ${CORE_INCLUDES})

# review_print: a reviews.json read with the core's reader and the insight's reading of it; --make writes seeded ones
# for the simulator (TP8).
add_executable(review_print
    "${TOOLS_DIR}/review_print/review_print.cpp"
    "${CORE_LIBS}/Sources/ReviewLog.cpp"
    "${CORE_LIBS}/Sources/ReviewInsight.cpp"
    "${CORE_LIBS}/Sources/SessionReview.cpp"
    ${SDK_JSON_SOURCES}
)
target_include_directories(review_print PRIVATE ${CORE_INCLUDES})

enable_testing()
add_test(NAME hybridx-training-host-tests COMMAND hybridx-training-host-tests)
# The generated plan header must match the plan files.
add_test(NAME plan-data-current
         COMMAND "${Python3_EXECUTABLE}" "${TOOLS_DIR}/Plans/gen_plans.py" --check)
