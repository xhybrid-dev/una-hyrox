# Sources and include directories of the Energy Probe (x-energy/docs/PROBE.md):
#   Core/ -- pure C++ with no SDK: the modes, the schedule, the minute's figures, the night's
#            battery sums, the state file. Host-tested in ../../../../Tests/Host. What the GUI
#            uses (ProbeMode.hpp, Fixed.hpp) is header-only: the GUI links no library source.
#   App/  -- the service: the sensors per mode, the files, the sensor check, the GUI messages.
# Started from the Bike Probe's (hybridx-ride/Tools/Probe).
file(GLOB ENERGYPROBE_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/Core/Sources/*.cpp)
file(GLOB ENERGYPROBE_APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/App/Sources/*.cpp)

set(LIBS_SOURCES ${ENERGYPROBE_CORE_SOURCES} ${ENERGYPROBE_APP_SOURCES})
set(LIBS_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/Core/Header
    ${CMAKE_CURRENT_LIST_DIR}/App/Header
)
