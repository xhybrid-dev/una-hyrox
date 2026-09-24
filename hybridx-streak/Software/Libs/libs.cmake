# Source lists for the three parts of HybridX Streak's own code.
#
#   Core    pure C++, no SDK. Header-only wherever the GUI uses it: the GUI ELF
#           links no Libs sources (hybridx-race NOTES 3.4).
#   App     the Utility app's service and the Service <-> GUI message contract.
#   Glance  the glance's service.
#
# App and Glance each have their own Service.hpp and Service class, because the
# SDK's service entry point includes "Service.hpp" and constructs `Service`.
# Keeping them in separate directories means each build sees exactly one.

set(STREAK_CORE_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/Core/Header)
file(GLOB_RECURSE STREAK_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)

file(GLOB_RECURSE STREAK_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
set(STREAK_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
    ${STREAK_CORE_INCLUDE_DIRS})

file(GLOB_RECURSE STREAK_GLANCE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Glance/Sources/*.cpp)
set(STREAK_GLANCE_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Glance/Header
    ${STREAK_CORE_INCLUDE_DIRS})
