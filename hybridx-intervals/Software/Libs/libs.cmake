# Sources and include directories of HybridX Intervals' two libraries:
#   Core/ -- the pure workout core (model, file parser, step engine, targets):
#            no kernel, host-tested in ../../Tests/Host;
#   App/  -- the service process, started from the SDK's RunLVGL.
# The throwaway P0 probe (../../Tools/Probe) has its own build.
file(GLOB INTERVALS_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB INTERVALS_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES ${INTERVALS_CORE_SOURCES} ${INTERVALS_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
