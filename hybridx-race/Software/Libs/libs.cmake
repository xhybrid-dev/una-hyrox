file(GLOB_RECURSE LIBS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/Sources/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Sources/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.c
    ${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp
)

# Link/ is a byte-identical copy of hybridx-link's code: the activity summary Streak
# reads (hybridx-link/docs/ACTIVITY_LINK.md), kept so by
# hybridx-link/Utilities/check-link-copies.sh.
file(GLOB RACE_LINK_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)
list(APPEND LIBS_SOURCES ${RACE_LINK_SOURCES})

set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
)