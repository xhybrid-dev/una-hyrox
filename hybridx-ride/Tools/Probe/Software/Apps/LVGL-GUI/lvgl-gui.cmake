# Sources and include directories of the bike probe's LVGL GUI process (Knitting's, from the
# SDK's Waypoint tutorial). The generated font C files under assets/ are
# committed, so no converter is needed to build.
file(GLOB_RECURSE GUI_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/assets/fonts/*.c
    ${CMAKE_CURRENT_LIST_DIR}/assets/images/*.c
)

set(GUI_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
)
