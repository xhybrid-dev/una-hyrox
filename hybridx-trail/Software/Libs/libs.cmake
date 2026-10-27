# Sources and include directories of HybridX Trail's two libraries:
#   Core/ -- the pure route core (GPX, thinning, tracking, alerts, map
#            projection): no kernel, host-tested in ../../Tests/Host;
#   App/  -- the service process, started from the SDK's RunLVGL;
#   Vo2Link/ -- copies of HybridX Run's window builder and file codec, so a
#            Trail run can count towards Run's VO2max (see Vo2Link/README.md);
#   Link/ -- a byte-identical copy of hybridx-link's code: the activity summary
#            HybridX Streak reads (hybridx-link/docs/ACTIVITY_LINK.md). Edit the
#            original, then hybridx-link/Utilities/check-link-copies.sh --sync.
# The throwaway T0 probe (../../Tools/Probe) has its own build.
file(GLOB TRAIL_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB TRAIL_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
file(GLOB TRAIL_VO2LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Vo2Link/Sources/*.cpp)
file(GLOB TRAIL_LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)

set(LIBS_SOURCES ${TRAIL_CORE_SOURCES} ${TRAIL_APP_SOURCES} ${TRAIL_VO2LINK_SOURCES} ${TRAIL_LINK_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
    ${CMAKE_CURRENT_LIST_DIR}/Vo2Link/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
)
