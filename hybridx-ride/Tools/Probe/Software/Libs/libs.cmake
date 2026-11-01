# Sources and include directories of the bike probe (hybridx-ride/docs/PROBE.md):
#   Core/ -- pure C++, header-only (the GUI shows its lines), host-tested in
#            ../../../Tests/Host;
#   App/  -- the service: the accessory requests, the status events, the CSV.
# Started from Knitting's skeleton, the smallest of the HybridX apps.
file(GLOB RIDEPROBE_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES ${RIDEPROBE_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
