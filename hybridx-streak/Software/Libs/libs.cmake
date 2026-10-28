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

# Link/ is a byte-identical copy of hybridx-link's code (the summaries the HybridX
# apps leave for Streak; hybridx-link/docs/ACTIVITY_LINK.md), kept so by
# hybridx-link/Utilities/check-link-copies.sh. LinkReader (Core) uses it, so it
# joins Core's lists and every build that has Core has it.
set(STREAK_CORE_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/Core/Header ${CMAKE_CURRENT_LIST_DIR}/Link/Header)
file(GLOB_RECURSE STREAK_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
# The FIT reader checks CRCs with the SDK's own function. (The JSON classes the
# state file uses come with UNA_SDK_SOURCES_SERVICE.)
list(APPEND STREAK_CORE_SOURCES "$ENV{UNA_SDK}/Libs/Source/Fit/FitCrc.cpp"
    ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/ActivityLink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/StreakNow.cpp)

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
