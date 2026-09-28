# Sources and include directories of HybridX Trail's two libraries:
#   Core/ -- the pure route core (GPX, thinning, tracking, alerts, map
#            projection): no kernel, host-tested in ../../Tests/Host;
#   App/  -- the service process, started from the SDK's RunLVGL.
# The throwaway T0 probe (../../Tools/Probe) has its own build.
file(GLOB TRAIL_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB TRAIL_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES ${TRAIL_CORE_SOURCES} ${TRAIL_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
