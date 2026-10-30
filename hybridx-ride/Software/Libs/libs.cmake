# Sources and include directories of HybridX Ride's libraries:
#   Core/ -- pure C++, no kernel, host-tested in ../../Tests/Host. Anything the
#            GUI uses is header-only, because the GUI ELF links no Libs
#            sources (hybridx-race NOTES 3.4); the settings file uses the
#            SDK's JSON reader and writer and is the service's alone;
#   App/  -- the app's service process and the Service <-> GUI message
#            contract (from X Sauna's);
#   Link/ -- a byte-identical copy of hybridx-link's code: the activity summary
#            Streak reads, and the Streak line (hybridx-link/docs/ACTIVITY_LINK.md).
#            Edit the original, then hybridx-link/Utilities/check-link-copies.sh --sync.
file(GLOB RIDE_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB RIDE_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
file(GLOB RIDE_LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)

set(LIBS_SOURCES ${RIDE_CORE_SOURCES} ${RIDE_APP_SOURCES} ${RIDE_LINK_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
)
