# Sources and include directories of HybridX Run's two libraries:
#   Core/ -- the pure VO2max core (profile, windows, run estimate, history):
#            no kernel (only the SDK's JSON reader and writer), host-tested in
#            ../../Tests/Host;
#   App/  -- the service process, started from the SDK's RunLVGL.
file(GLOB RUN_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB RUN_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)
# The GUI process needs only the text (Vo2Text: numbers and reasons) and the
# pickers' steps (Vo2Settings).
set(RUN_CORE_GUI_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/Vo2Text.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/Vo2Settings.cpp
)

set(LIBS_SOURCES ${RUN_CORE_SOURCES} ${RUN_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
