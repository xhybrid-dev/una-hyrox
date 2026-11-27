# Sources and include directories of the Clock Probe (x-clock/docs/PROBE.md):
#   Core/     -- the probe's own pure C++: what to look for in the shared folder, the week view,
#                the wrist and frame logs, the text of every log row and screen line. Host-tested in
#                ../../../Tests/Host. What the GUI uses (ProbeView.hpp, CivilDate.hpp, Fixed.hpp) is
#                header-only: the GUI links no library source.
#   Link/     -- a COPY of hybridx-link's activity link (hybridx-link/Utilities/check-link-copies.sh).
#   Training/ -- COPIES of X Training's training_today.json reader (x-clock/Utilities/check-copies.sh).
#   Vo2/      -- COPIES of HybridX Run's vo2max.json reader (the same script).
#   App/      -- the service: the sensors, the files, the scan, the GUI messages.
# Started from the Energy Probe's (x-energy/Tools/Probe).
file(GLOB CLOCKPROBE_CORE_SOURCES     CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB CLOCKPROBE_LINK_SOURCES     CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Link/Sources/*.cpp)
file(GLOB CLOCKPROBE_TRAINING_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Training/Sources/*.cpp)
file(GLOB CLOCKPROBE_VO2_SOURCES      CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Vo2/Sources/*.cpp)
file(GLOB CLOCKPROBE_APP_SOURCES      CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES
    ${CLOCKPROBE_CORE_SOURCES}
    ${CLOCKPROBE_LINK_SOURCES}
    ${CLOCKPROBE_TRAINING_SOURCES}
    ${CLOCKPROBE_VO2_SOURCES}
    ${CLOCKPROBE_APP_SOURCES}
)
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/Link/Header
    ${CMAKE_CURRENT_LIST_DIR}/Training/Header
    ${CMAKE_CURRENT_LIST_DIR}/Vo2/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
