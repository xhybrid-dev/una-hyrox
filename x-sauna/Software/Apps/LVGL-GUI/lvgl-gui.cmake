# Sources and include directories of X Sauna's LVGL GUI process (from the SDK's
# Waypoint tutorial, through Knitting). The generated font C files under assets/
# are committed, so no converter is needed to build.
file(GLOB_RECURSE GUI_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/assets/fonts/*.c
    ${CMAKE_CURRENT_LIST_DIR}/assets/images/*.c
)

set(GUI_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
)
