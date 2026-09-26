/**
 ******************************************************************************
 * @file    GpxReader.hpp
 * @brief   Reads route and track points out of a GPX file, a chunk at a time.
 *
 * A GPX file is XML, and a long trail route can run to megabytes, far more
 * than an app should hold. So this reads it as a stream: feed() takes
 * whatever chunk the file read returned, of any size, split anywhere (in the
 * middle of a tag, an attribute or a number), and hands each point to a
 * GpxSink as soon as its closing tag arrives. Memory is fixed: one 256-byte
 * tag buffer and one 64-byte text buffer, whatever the file's size.
 *
 * It is not a general XML parser, and does not try to be. It knows exactly
 * what a GPX 1.0/1.1 route needs:
 *
 *   - <trkpt lat=".." lon=".."> (a track: what Strava, Komoot, Garmin and most
 *     recorded activities export) and <rtept ..> (a route: what OS Maps and
 *     some planners export), each with an optional <ele> in metres;
 *   - the first <name> outside a point or waypoint, for the route list;
 *   - namespace prefixes (<gpx:trkpt>) are ignored, single or double quotes
 *     both work, comments, <?..?> declarations and CDATA are skipped over
 *     (CDATA text is kept if it is the name).
 *
 * Waypoints (<wpt>) are counted but not passed on: they are not part of the
 * line to follow. Everything else (extensions, times, heart rate, metadata)
 * is skipped without being stored.
 *
 * A point whose lat/lon is missing, malformed or out of range is counted in
 * Stats::badPoints and dropped, never passed on: a malformed file gives fewer
 * points or none, never a wrong one.
 ******************************************************************************
 */

#ifndef TRAIL_GPX_READER_HPP
#define TRAIL_GPX_READER_HPP

#include <cstddef>
#include <cstdint>

#include "GeoPoint.hpp"

namespace Trail
{

enum class PointKind : uint8_t { Track, Route };

/// Where points go, in file order.
class GpxSink
{
public:
    /// @param eleCm elevation in centimetres, meaningful only when @p hasEle.
    virtual void point(const GeoPoint& p, bool hasEle, int32_t eleCm, PointKind kind) = 0;

protected:
    ~GpxSink() = default;
};

class GpxReader
{
public:
    static constexpr size_t kTagMax  = 256;
    static constexpr size_t kTextMax = 64;
    static constexpr size_t kNameMax = 48;

    struct Stats {
        bool     sawGpx       = false;   ///< a <gpx> element was seen: it looks like GPX
        uint32_t trackPoints  = 0;       ///< good <trkpt>s passed on
        uint32_t routePoints  = 0;       ///< good <rtept>s passed on
        uint32_t waypoints    = 0;       ///< <wpt>s seen (not passed on)
        uint32_t badPoints    = 0;       ///< points dropped: missing or malformed lat/lon
        uint32_t longTags     = 0;       ///< tags longer than kTagMax (their tail was not read)
        char     name[kNameMax] = {};    ///< the first route/track/file name, UTF-8 as in the file
    };

    explicit GpxReader(GpxSink& sink);

    /// Start again, for a new file.
    void reset();

    /// Any number of bytes, split anywhere.
    void feed(const char* data, size_t len);

    const Stats& stats() const { return mStats; }

    /// Parse "  -3.1234567 " into value x 10^digits, rounded half away from
    /// zero. Leading/trailing blanks are allowed; anything else (exponents,
    /// commas, a second point, no digits) is refused. Public for the tests.
    static bool parseFixed(const char* s, size_t len, unsigned digits, int64_t& out);

private:
    enum class State : uint8_t { Text, Tag };
    enum class Capture : uint8_t { None, Ele, Name };

    void appendTag(char c);
    void endTag();
    void openPoint(PointKind kind, bool selfClosing);
    void closePoint();
    bool attribute(const char* key, const char*& value, size_t& len) const;
    void appendText(const char* s, size_t n);
    void finishName();

    GpxSink& mSink;
    Stats    mStats {};

    State   mState   = State::Text;
    char    mQuote   = 0;       ///< inside a quoted attribute value in a tag
    char    mTag[kTagMax] {};
    size_t  mTagLen  = 0;       ///< bytes stored (capped at kTagMax - 1)
    size_t  mTagSeen = 0;       ///< bytes seen, stored or not
    char    mTail[2] {};        ///< the tag's last two bytes, for "-->" and "]]>"

    Capture mCapture = Capture::None;
    char    mText[kTextMax] {};
    size_t  mTextLen = 0;

    bool      mInPoint  = false;
    PointKind mKind     = PointKind::Track;
    bool      mPointOk  = false;
    GeoPoint  mPoint {};
    bool      mHasEle   = false;
    int32_t   mEleCm    = 0;
    uint16_t  mWptDepth = 0;    ///< inside a <wpt>: its <name> is not the route's
    bool      mHaveName = false;
};

} // namespace Trail

#endif // TRAIL_GPX_READER_HPP
