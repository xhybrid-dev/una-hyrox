# Sources and include directories of X Weigh-in's three libraries:
#   Core/   -- pure C++, no kernel, host-tested in ../../Tests/Host. Anything the
#              GUI uses is header-only, because the GUI ELF links no Libs
#              sources (hybridx-race NOTES 3.4);
#   App/    -- the app's service process and the Service <-> GUI message
#              contract, started from the SDK's Waypoint tutorial;
#   Glance/ -- the glance's service (a second .uapp) and its pure layout.
#
# App and Glance each have their own Service.hpp and Service class, because the
# SDK's service entry point includes "Service.hpp" and constructs `Service`
# (as HybridX Streak). Keeping them in separate directories means each build
# sees exactly one.
file(GLOB WEIGHIN_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB WEIGHIN_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
file(GLOB WEIGHIN_GLANCE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Glance/Sources/*.cpp)

# The app (XWeighIn-CMake): Core + App.
set(LIBS_SOURCES ${WEIGHIN_CORE_SOURCES} ${WEIGHIN_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)

# The glance (XWeighInGlance-CMake): Core + Glance.
set(WEIGHIN_GLANCE_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Glance/Header
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
)
