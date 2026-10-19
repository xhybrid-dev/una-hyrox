# Sources and include directories of Knitting' two libraries:
#   Core/ -- pure C++, no kernel, host-tested in ../../Tests/Host. Anything the
#            GUI uses is header-only, because the GUI ELF links no Libs
#            sources (hybridx-race NOTES 3.4);
#   App/  -- the app's service process and the Service <-> GUI message
#            contract, started from the SDK's Waypoint tutorial.
file(GLOB KNITTING_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB KNITTING_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES ${KNITTING_CORE_SOURCES} ${KNITTING_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
