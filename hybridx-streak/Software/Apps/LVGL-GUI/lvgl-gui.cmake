# Sources and include directories of HybridX Streak's GUI process. Generated
# font and image C files live under assets/ and are committed.
file(GLOB_RECURSE GUI_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/assets/fonts/*.c
    ${CMAKE_CURRENT_LIST_DIR}/assets/images/*.c
)

set(GUI_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
    ${CMAKE_CURRENT_LIST_DIR}/assets
)

# The design demo: every screen driven by canned scenarios instead of the
# service's real data, for design review and screenshot captures. Off since the
# service provides real data (phase S2); the capture scripts build a separate
# demo simulator with -DHYBRIDXSTREAK_DEMO=ON.
option(HYBRIDXSTREAK_DEMO "Drive the GUI from demo scenarios" OFF)
if(HYBRIDXSTREAK_DEMO)
    set(STREAK_GUI_DEFINES HYBRIDXSTREAK_DEMO=1)
endif()
