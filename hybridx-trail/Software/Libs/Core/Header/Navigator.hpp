/**
 ******************************************************************************
 * @file    Navigator.hpp
 * @brief   The route library and the live navigation, over IFileSystem.
 *
 * Everything the service does with routes, kept out of the service so the
 * host tests can run it against an in-memory file system:
 *
 *   - scan(): lists the GPX files in Routes/ (copied there over USB) and
 *     summarises each: its name, length and climb. Parsing a long GPX takes
 *     about a third of a second on the watch (NOTES, Gate T0), so summaries are
 *     kept in routes.idx and a file is parsed again only when its size or date
 *     changes. The list is sorted by name.
 *   - load(): reads one route into the fixed point array, remembers the choice
 *     in route.sel for next time, and points the tracker at it.
 *   - update(): one GPS fix: where the runner is along the route, the heading
 *     (GPS while running, compass when slow: HeadingFusion), the off-course
 *     alert, the way back to the line, and the next turn (TurnFinder). Alerts
 *     and turn cues are only raised while @p alertsLive (an activity
 *     running): walking about on the start screen, or paused, is not being
 *     lost.
 *   - the elevation profile of the loaded route (ElevationProfile).
 *
 * Memory is fixed: 2,000 route points (16 KB), their cumulative distances
 * (8 KB), their elevation and climb (8 KB) and 16 route summaries. Construct it once, in static storage: it is
 * far too big for the service's 10 KB stack.
 ******************************************************************************
 */

#ifndef TRAIL_NAVIGATOR_HPP
#define TRAIL_NAVIGATOR_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "CourseOverGround.hpp"
#include "ElevationProfile.hpp"
#include "GeoPoint.hpp"
#include "GpxReader.hpp"
#include "HeadingFusion.hpp"
#include "OffCourse.hpp"
#include "RouteBuilder.hpp"
#include "RouteTracker.hpp"
#include "TurnFinder.hpp"

namespace Trail
{

struct RouteInfo {
    char     file[48]  = {};   ///< file name in Routes/
    char     name[32]  = {};   ///< the name inside the GPX, or the file name without ".gpx"
    uint32_t bytes     = 0;
    uint32_t utc       = 0;    ///< the file's date, as the file system reports it
    uint32_t lengthM   = 0;
    uint16_t ascentM   = 0;
    uint16_t points    = 0;    ///< kept after thinning; 0 = unreadable
};

class Navigator
{
public:
    static constexpr uint16_t    kMaxPoints = 2000;
    static constexpr uint8_t     kMaxRoutes = 16;
    static constexpr const char* kRoutesDir = "Routes";
    static constexpr const char* kIndexFile = "routes.idx";
    static constexpr const char* kSelFile   = "route.sel";

    static constexpr float kGuideMinM   = 25.0f;    ///< show the way back from this far off the line
    static constexpr float kCueM        = 50.0f;    ///< a turn is announced this far ahead
    static constexpr float kLookaheadM  = 400.0f;   ///< and shown from this far

    struct Status {
        bool                   routeLoaded  = false;
        bool                   hasFix       = false;
        GeoPoint               fix {};
        float                  precisionM   = 0.0f;
        bool                   headingValid = false;
        float                  headingDeg   = 0.0f;
        RouteTracker::Position pos {};
        OffCourse::State       alert        = OffCourse::State::NotStarted;
        uint32_t               offForS      = 0;
        float                  toStartM     = 0.0f;   ///< straight-line distance to the route's start
        uint8_t                headingSource = 0;     ///< HeadingFusion::Source: 0 none, 1 GPS, 2 compass
        // The way back: to the nearest part of the route when off it, or to the start before joining it.
        bool                   guideValid   = false;
        bool                   guideToStart = false;
        float                  guideBearingDeg = 0.0f;   ///< true bearing from the fix
        float                  guideDistM   = 0.0f;
        // The next turn ahead, if within kLookaheadM.
        bool                   turnValid    = false;
        int16_t                turnAngleDeg = 0;         ///< positive right, negative left
        uint16_t               turnDistM    = 0;
    };

    explicit Navigator(SDK::Interface::IFileSystem& fs);

    /// List Routes/ (creating it if missing). Returns how many routes there are.
    uint8_t          scan();
    uint8_t          routeCount() const { return mRouteCount; }
    const RouteInfo* routes() const { return mRoutes; }

    /// Load route @p index from the last scan(). False (and no route) if it
    /// cannot be read.
    bool load(uint8_t index);
    /// Load the route chosen last time, if it is still in Routes/.
    bool restoreSelection();
    /// Navigate with no route (a plain run).
    void clear();

    bool             loaded() const { return mStatus.routeLoaded; }
    int8_t           selected() const { return mSelected; }   ///< -1 when no route
    const RouteInfo& current() const { return mCurrent; }
    const GeoPoint*  points() const { return mPoints; }
    uint16_t         pointCount() const { return mPointCount; }

    /// Forget progress along the route (a new activity starts).
    void resetProgress();

    /// One GPS fix. Returns the alert to raise (None unless @p alertsLive).
    OffCourse::Event update(uint32_t nowMs, const GeoPoint& fix, float precisionM, bool alertsLive);

    /// GPS lost: the position is stale until the next update().
    void lostFix() { mStatus.hasFix = false; }

    /// The latest compass reading (12 o'clock's bearing from magnetic north),
    /// @p valid only for a calibrated, current sample. Then refreshHeading().
    void setCompass(bool valid, float bearingDeg)
    {
        mCompassValid = valid;
        mCompassDeg   = bearingDeg;
    }
    /// The GPS ground speed, if the receiver gave a valid one; else the speed
    /// is estimated from the fixes.
    void setSpeed(bool valid, float mps)
    {
        mSpeedValid = valid;
        mSpeedMps   = mps;
    }
    /// Recompute the heading from the latest GPS course, speed and compass.
    /// update() does; call it too when only the compass has changed.
    void refreshHeading();

    /// A turn to announce (once each), while an activity runs. Angle: positive
    /// right, negative left.
    bool takeTurnCue(int16_t& angleDeg);

    const ElevationProfile& profile() const { return mProfile; }
    const RouteTracker&     tracker() const { return mTracker; }

    const Status& status() const { return mStatus; }

    /// How many GPX files have been parsed (for the tests: the index should
    /// make a second scan() parse nothing).
    uint16_t parses() const { return mParses; }

private:
    bool parse(const char* file, RouteInfo& info);
    void loadIndex();
    void saveIndex();
    void writeSelection(const char* file);
    void updateGuide();
    void updateTurn(bool alertsLive);

    SDK::Interface::IFileSystem& mFs;
    RouteBuilder                 mBuilder;   ///< before mReader, which feeds it
    GpxReader                    mReader;
    RouteTracker                 mTracker;
    OffCourse                    mOffCourse;
    CourseOverGround             mCourse;
    HeadingFusion                mHeading;
    ElevationProfile             mProfile;

    GeoPoint  mPoints[kMaxPoints] {};
    float     mCumulative[kMaxPoints] {};
    int16_t   mEleHalf[kMaxPoints] {};
    uint16_t  mAscent[kMaxPoints] {};

    bool      mCompassValid = false;
    float     mCompassDeg   = 0.0f;
    bool      mSpeedValid   = false;
    float     mSpeedMps     = 0.0f;
    float     mEstSpeedMps  = 0.0f;   ///< from the fixes, when the GPS gave none
    GeoPoint  mPrevFix {};
    uint32_t  mPrevFixMs    = 0;
    bool      mHavePrevFix  = false;
    float     mLastCueAlong = -1.0f;
    bool      mCuePending   = false;
    int16_t   mCueAngle     = 0;
    uint16_t  mPointCount = 0;
    RouteInfo mRoutes[kMaxRoutes] {};
    uint8_t   mRouteCount = 0;
    RouteInfo mCached[kMaxRoutes] {};   ///< routes.idx, as read
    uint8_t   mCachedCount = 0;
    RouteInfo mCurrent {};
    int8_t    mSelected = -1;
    Status    mStatus {};
    uint16_t  mParses = 0;

    SDK::Interface::IFileSystem::ObjectInfo mInfo {};
    char mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char mChunk[512] {};
};

} // namespace Trail

#endif // TRAIL_NAVIGATOR_HPP
