# Sources and include directories of X Training's libraries:
#   Core/    -- X Training's pure plan core: paces, the plans (generated from
#               ../../Tools/Plans), placing a week, the calendar, the session
#               builder and the plan state. Host-tested in ../../Tests/Host.
#   RunCore/ -- X Run's core, copied at c9b6618: kept for the run statistics
#               (zones, best splits), running form and the treadmill model.
#   App/     -- the service process: X Run's, itself from the SDK's RunLVGL.
#   Link/    -- the HybridX activity link (the summary Streak reads): a
#               byte-identical copy of hybridx-link's code, kept so by
#               hybridx-link/Utilities/check-link-copies.sh.
file(GLOB XT_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB RUN_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/RunCore/Sources/*.cpp)
file(GLOB RUN_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
file(GLOB RUN_LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)

# The GUI process links none of the libraries' sources except these: what it
# uses of the cores is header-only (Core's Date, TimeText, PlanText, PlanTypes
# and data/PlanData; RunCore's text headers).
set(RUN_CORE_GUI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/RunCore/Sources/Vo2Text.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RunCore/Sources/Vo2Settings.cpp
)

set(LIBS_SOURCES ${XT_CORE_SOURCES} ${RUN_CORE_SOURCES} ${RUN_APP_SOURCES} ${RUN_LINK_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/RunCore/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
)
