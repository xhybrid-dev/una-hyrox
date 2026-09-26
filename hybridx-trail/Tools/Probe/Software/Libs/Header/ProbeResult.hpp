/**
 ******************************************************************************
 * @file    ProbeResult.hpp
 * @brief   What the Trail Probe has found so far, in a fixed-size struct.
 *
 * Header-only and trivially copyable: the service fills it, sends it to the
 * GUI once a second inside a message, and formats the report from it.
 * Positions are deliberately not in here: the GUI has no use for them, and
 * the report prints them rounded (Service.cpp).
 ******************************************************************************
 */

#ifndef TRAIL_PROBE_RESULT_HPP
#define TRAIL_PROBE_RESULT_HPP

#include <cstdint>
#include <type_traits>

namespace Probe
{

/// The route half of Gate T0 (hybridx-trail PLAN T0): did a GPX copied into
/// Routes/ over USB reach the app, and could it be read into a route?
enum class Verdict : uint8_t {
    Running,       ///< not finished yet
    Go,            ///< a .gpx was found in Routes/ and read into a route
    NoRoute,       ///< Routes/ is there, but holds no .gpx file
    Unreadable,    ///< a .gpx was found, but it could not be opened or held no points
    FolderFailed,  ///< Routes/ itself could not be created or opened
};

/// A sensor's progress during the probe's sampling window.
enum class Sense : uint8_t {
    Waiting,       ///< not started (the GUI hasn't opened yet)
    NoData,        ///< connected, nothing has arrived yet
    Searching,     ///< GPS: samples arriving, no fix yet. Compass: samples, no calibration
    Ok,            ///< GPS: a fix. Compass: a calibrated bearing
    ConnectFailed, ///< the sensor layer refused the connection
};

struct Result {
    // -- Routes/ (Probe::Runner) ------------------------------------------------------
    Verdict  verdict      = Verdict::Running;
    bool     looksGpx     = false;   ///< a <gpx> root element was seen
    bool     hasEle       = false;
    uint16_t gpxCount     = 0;       ///< .gpx files in Routes/
    uint16_t skipped      = 0;       ///< other entries (folders, hidden "._" files, other types)
    char     file[40]     = {};      ///< the newest .gpx's file name (ASCII-folded)
    char     name[40]     = {};      ///< the name inside it, if any (ASCII-folded)
    uint32_t fileBytes    = 0;
    uint32_t bytesRead    = 0;
    uint32_t rawPoints    = 0;       ///< points in the file (the kind used)
    uint32_t badPoints    = 0;       ///< points with missing or malformed coordinates
    uint32_t ignored      = 0;       ///< points of the other kind (track vs route)
    uint16_t kept         = 0;       ///< points kept after thinning
    uint16_t spacingM     = 0;       ///< final thinning spacing
    uint32_t lengthM      = 0;
    uint32_t ascentM      = 0;
    uint32_t readMs       = 0;       ///< listing plus reading, watch time

    // -- Memory ------------------------------------------------------------------------
    uint32_t largestAllocB = 0;      ///< the largest single block the service could allocate

    // -- Live sensors (the service's sampling window) ---------------------------------
    uint16_t elapsedS     = 0;       ///< seconds since sampling started
    bool     finished     = false;   ///< the sampling window is over
    Sense    gps          = Sense::Waiting;
    uint16_t fixAfterS    = 0;       ///< seconds from start to the first fix (0 = none yet)
    uint16_t precisionDm  = 0;       ///< the GPS's own precision estimate, decimetres
    uint32_t gpsSamples   = 0;
    Sense    compass      = Sense::Waiting;
    uint32_t magSamples   = 0;
    uint32_t magCalibrated = 0;      ///< samples with MAG_CALIBRATED set
    int16_t  bearingDeg   = -1;      ///< level bearing, magnetic, -1 = none
    int16_t  tiltedDeg    = -1;      ///< tilt-compensated bearing, -1 = none
    uint32_t accelSamples = 0;
    int32_t  offRouteM    = -1;      ///< metres from the loaded route, -1 = not known

    uint16_t run          = 0;       ///< this run's number, from probe-history.txt
};

static_assert(std::is_trivially_copyable<Result>::value, "Result travels in a message");

inline const char* verdictName(Verdict v)
{
    switch (v) {
        case Verdict::Running:      return "running";
        case Verdict::Go:           return "GO";
        case Verdict::NoRoute:      return "NO ROUTE";
        case Verdict::Unreadable:   return "UNREADABLE";
        case Verdict::FolderFailed: return "FOLDER FAILED";
    }
    return "?";
}

inline const char* senseName(Sense s)
{
    switch (s) {
        case Sense::Waiting:       return "waiting";
        case Sense::NoData:        return "no data";
        case Sense::Searching:     return "searching";
        case Sense::Ok:            return "ok";
        case Sense::ConnectFailed: return "connect failed";
    }
    return "?";
}

} // namespace Probe

#endif // TRAIL_PROBE_RESULT_HPP
