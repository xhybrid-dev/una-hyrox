# Sources and include directories of HybridX Run's libraries:
#   Core/ -- the pure VO2max core (profile, windows, run estimate, history):
#            no kernel (only the SDK's JSON reader and writer), host-tested in
#            ../../Tests/Host;
#   App/  -- the service process, started from the SDK's RunLVGL;
#   Glance/ -- the VO2max glance's service (a second .uapp) and its pure
#            view and layout (R6).
#
# App and Glance each have their own Service.hpp and Service class, because the
# SDK's service entry point includes "Service.hpp" and constructs `Service`
# (as HybridX Streak and X Weigh-in). Keeping them in separate directories
# means each build sees exactly one.
file(GLOB RUN_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB RUN_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
# The GUI process needs only the text (Vo2Text: numbers and reasons) and the
# pickers' steps (Vo2Settings).
set(RUN_CORE_GUI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/Vo2Text.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/Vo2Settings.cpp
)

# Link/ -- the HybridX activity link (the summary Streak reads): a byte-identical
#          copy of hybridx-link/'s code, kept so by Utilities/check-link-copies.sh.
file(GLOB RUN_LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)

set(LIBS_SOURCES ${RUN_CORE_SOURCES} ${RUN_APP_SOURCES} ${RUN_LINK_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
)

# The glance (HybridXRunGlance-CMake): Glance + the text it formats with.
file(GLOB RUN_GLANCE_LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Glance/Sources/*.cpp)
set(RUN_GLANCE_SOURCES
    ${RUN_GLANCE_LIB_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/Vo2Text.cpp
)
set(RUN_GLANCE_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Glance/Header
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
)
