/**
 ******************************************************************************
 * @file    Navigator.cpp
 * @brief   The route library and the live navigation (see the header).
 ******************************************************************************
 */

#include "Navigator.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using SDK::Interface::IFileSystem;

namespace Trail
{

namespace
{
constexpr uint16_t kMaxEntries  = 200;         ///< bounds the directory read
constexpr uint32_t kMaxGpxBytes = 16u << 20;   ///< stop reading a GPX past 16 MB
constexpr const char* kIndexHeader = "HXTRAIL-IDX 1";

bool isGpx(const char* name)
{
    const size_t n = std::strlen(name);
    if (n < 5 || name[0] == '.') {   // hidden, including macOS "._name.gpx"
        return false;
    }
    const char* e = name + n - 4;
    return e[0] == '.' && (e[1] | 0x20) == 'g' && (e[2] | 0x20) == 'p' && (e[3] | 0x20) == 'x';
}

void copyText(char* out, size_t cap, const char* in)
{
    std::strncpy(out, in, cap - 1);
    out[cap - 1] = '\0';
}

/// Case-insensitive, for the route list's order.
int compareNames(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        char ca = *a >= 'A' && *a <= 'Z' ? static_cast<char>(*a + 32) : *a;
        char cb = *b >= 'A' && *b <= 'Z' ? static_cast<char>(*b + 32) : *b;
        if (ca != cb || ca == '\0') {
            return static_cast<unsigned char>(ca) - static_cast<unsigned char>(cb);
        }
    }
}

bool sameFile(const RouteInfo& a, const RouteInfo& b)
{
    return std::strcmp(a.file, b.file) == 0 && a.bytes == b.bytes && a.utc == b.utc;
}
} // namespace

Navigator::Navigator(IFileSystem& fs)
    : mFs(fs)
    , mBuilder(mPoints, kMaxPoints)
    , mReader(mBuilder)
{
}

// -- The library ------------------------------------------------------------------

uint8_t Navigator::scan()
{
    mRouteCount = 0;
    mFs.mkdir(kRoutesDir);
    auto dir = mFs.dir(kRoutesDir);
    if (dir && dir->open()) {
        uint16_t seen = 0;
        while (seen < kMaxEntries && mRouteCount < kMaxRoutes && dir->readNext(mInfo)) {
            ++seen;
            if (mInfo.isDir || !isGpx(mInfo.name) || std::strlen(mInfo.name) >= sizeof(RouteInfo::file)) {
                continue;
            }
            RouteInfo& r = mRoutes[mRouteCount++];
            r            = RouteInfo {};
            copyText(r.file, sizeof(r.file), mInfo.name);
            r.bytes = static_cast<uint32_t>(mInfo.size);
            r.utc   = static_cast<uint32_t>(mInfo.utc);
        }
        dir->close();
    }

    // Summaries: from routes.idx where the file is unchanged, else parse it.
    loadIndex();
    bool parsedAny = false;
    for (uint8_t i = 0; i < mRouteCount; ++i) {
        bool cached = false;
        for (uint8_t c = 0; c < mCachedCount; ++c) {
            if (sameFile(mCached[c], mRoutes[i])) {
                mRoutes[i] = mCached[c];
                cached     = true;
                break;
            }
        }
        if (!cached) {
            parse(mRoutes[i].file, mRoutes[i]);
            parsedAny = true;
        }
    }

    // By name: what the runner reads in the list.
    for (uint8_t i = 1; i < mRouteCount; ++i) {
        const RouteInfo moving = mRoutes[i];
        uint8_t         j      = i;
        while (j > 0 && compareNames(mRoutes[j - 1].name, moving.name) > 0) {
            mRoutes[j] = mRoutes[j - 1];
            --j;
        }
        mRoutes[j] = moving;
    }

    if (parsedAny || mCachedCount != mRouteCount) {
        saveIndex();
    }

    // Parsing used the point array: put the loaded route back, or follow it
    // to its new place in the list.
    if (mStatus.routeLoaded) {
        int8_t found = -1;
        for (uint8_t i = 0; i < mRouteCount; ++i) {
            if (sameFile(mRoutes[i], mCurrent)) {
                found = static_cast<int8_t>(i);
            }
        }
        if (found < 0) {
            clear();   // deleted or changed since it was loaded
        } else if (parsedAny) {
            load(static_cast<uint8_t>(found));
        } else {
            mSelected = found;
        }
    }
    return mRouteCount;
}

bool Navigator::parse(const char* file, RouteInfo& info)
{
    ++mParses;
    info.points  = 0;
    info.lengthM = 0;
    info.ascentM = 0;
    if (std::snprintf(mPath, sizeof(mPath), "%s/%s", kRoutesDir, file) >= static_cast<int>(sizeof(mPath))) {
        return false;
    }
    auto f = mFs.file(mPath);
    if (!f || !f->open(false, false)) {
        return false;
    }
    mReader.reset();
    mBuilder.reset();
    uint32_t total = 0;
    size_t   got   = 0;
    while (total < kMaxGpxBytes && f->read(mChunk, sizeof(mChunk), got) && got > 0) {
        mReader.feed(mChunk, got);
        total += static_cast<uint32_t>(got);
    }
    f->close();
    mBuilder.finish();

    info.points  = mBuilder.count() >= 2 ? mBuilder.count() : 0;
    info.lengthM = mBuilder.lengthM();
    info.ascentM = static_cast<uint16_t>(mBuilder.ascentM() > 65535u ? 65535u : mBuilder.ascentM());
    if (mReader.stats().name[0] != '\0') {
        copyText(info.name, sizeof(info.name), mReader.stats().name);
    } else {
        copyText(info.name, sizeof(info.name), file);
        const size_t n = std::strlen(info.name);
        if (n >= 4 && info.name[n - 4] == '.') {
            info.name[n - 4] = '\0';   // "Lakes 20k.gpx" -> "Lakes 20k"
        }
    }
    return info.points > 0;
}

void Navigator::loadIndex()
{
    mCachedCount = 0;
    auto f       = mFs.file(kIndexFile);
    if (!f || !f->open(false, false)) {
        return;
    }
    // One line per route: file, bytes, utc, lengthM, ascentM, points, name,
    // tab-separated, under a header line. Anything unexpected: ignore the
    // rest (the routes are simply parsed again).
    char   line[160];
    size_t len    = 0;
    bool   header = false;
    size_t got    = 0;
    bool   done   = false;
    while (!done && f->read(mChunk, sizeof(mChunk), got) && got > 0) {
        for (size_t i = 0; i < got && !done; ++i) {
            const char c = mChunk[i];
            if (c != '\n') {
                if (len < sizeof(line) - 1) {
                    line[len++] = c;
                }
                continue;
            }
            line[len] = '\0';
            len       = 0;
            if (!header) {
                header = std::strcmp(line, kIndexHeader) == 0;
                done   = !header;
                continue;
            }
            if (mCachedCount >= kMaxRoutes) {
                done = true;
                continue;
            }
            char*     fields[7] = {};
            uint8_t   n         = 0;
            char*     p         = line;
            fields[n++]         = p;
            while (*p != '\0' && n < 7) {
                if (*p == '\t') {
                    *p          = '\0';
                    fields[n++] = p + 1;
                }
                ++p;
            }
            if (n != 7 || fields[0][0] == '\0') {
                done = true;
                continue;
            }
            RouteInfo& r = mCached[mCachedCount++];
            r            = RouteInfo {};
            copyText(r.file, sizeof(r.file), fields[0]);
            r.bytes   = static_cast<uint32_t>(std::strtoul(fields[1], nullptr, 10));
            r.utc     = static_cast<uint32_t>(std::strtoul(fields[2], nullptr, 10));
            r.lengthM = static_cast<uint32_t>(std::strtoul(fields[3], nullptr, 10));
            r.ascentM = static_cast<uint16_t>(std::strtoul(fields[4], nullptr, 10));
            r.points  = static_cast<uint16_t>(std::strtoul(fields[5], nullptr, 10));
            copyText(r.name, sizeof(r.name), fields[6]);
        }
    }
    f->close();
}

void Navigator::saveIndex()
{
    auto f = mFs.file(kIndexFile);
    if (!f || !f->open(true, true)) {
        return;
    }
    size_t written = 0;
    char   line[160];
    int    n = std::snprintf(line, sizeof(line), "%s\n", kIndexHeader);
    f->write(line, static_cast<size_t>(n), written);
    for (uint8_t i = 0; i < mRouteCount; ++i) {
        const RouteInfo& r = mRoutes[i];
        char             name[sizeof(r.name)];
        copyText(name, sizeof(name), r.name);
        for (char* c = name; *c != '\0'; ++c) {
            if (*c == '\t' || *c == '\n' || *c == '\r') {
                *c = ' ';
            }
        }
        n = std::snprintf(line, sizeof(line), "%s\t%lu\t%lu\t%lu\t%u\t%u\t%s\n", r.file,
                          static_cast<unsigned long>(r.bytes), static_cast<unsigned long>(r.utc),
                          static_cast<unsigned long>(r.lengthM), static_cast<unsigned>(r.ascentM),
                          static_cast<unsigned>(r.points), name);
        if (n > 0 && static_cast<size_t>(n) < sizeof(line)) {
            f->write(line, static_cast<size_t>(n), written);
        }
    }
    f->close();
}

// -- Choosing a route ---------------------------------------------------------------

bool Navigator::load(uint8_t index)
{
    if (index >= mRouteCount) {
        return false;
    }
    RouteInfo info = mRoutes[index];
    if (!parse(info.file, info)) {
        clear();
        return false;
    }
    mRoutes[index] = info;   // refresh the summary with what was read
    mPointCount    = mBuilder.count();
    mTracker.bind(mPoints, mPointCount, mCumulative, static_cast<float>(mBuilder.lengthM()));
    mCurrent            = info;
    mSelected           = static_cast<int8_t>(index);
    mStatus.routeLoaded = true;
    writeSelection(info.file);
    resetProgress();
    return true;
}

bool Navigator::restoreSelection()
{
    auto f = mFs.file(kSelFile);
    if (!f || !f->open(false, false)) {
        return false;
    }
    char   name[sizeof(RouteInfo::file)] = {};
    size_t got                           = 0;
    f->read(name, sizeof(name) - 1, got);
    f->close();
    name[got < sizeof(name) ? got : sizeof(name) - 1] = '\0';
    for (char* c = name; *c != '\0'; ++c) {
        if (*c == '\n' || *c == '\r') {
            *c = '\0';
            break;
        }
    }
    for (uint8_t i = 0; i < mRouteCount; ++i) {
        if (std::strcmp(mRoutes[i].file, name) == 0) {
            return load(i);
        }
    }
    return false;
}

void Navigator::clear()
{
    mPointCount = 0;
    mTracker.bind(nullptr, 0, nullptr);
    mCurrent            = RouteInfo {};
    mSelected           = -1;
    mStatus.routeLoaded = false;
    mStatus.pos         = RouteTracker::Position {};
    mStatus.alert       = OffCourse::State::NotStarted;
    mStatus.offForS     = 0;
    mStatus.toStartM    = 0.0f;
    mFs.remove(kSelFile);
    resetProgress();
}

void Navigator::writeSelection(const char* file)
{
    auto f = mFs.file(kSelFile);
    if (!f || !f->open(true, true)) {
        return;
    }
    size_t written = 0;
    f->write(file, std::strlen(file), written);
    f->close();
}

// -- On the move ----------------------------------------------------------------------

void Navigator::resetProgress()
{
    mTracker.reset();
    mOffCourse.reset();
    mCourse.reset();
    mStatus.pos     = mTracker.position();
    mStatus.alert   = mOffCourse.state();
    mStatus.offForS = 0;
}

OffCourse::Event Navigator::update(uint32_t nowMs, const GeoPoint& fix, float precisionM, bool alertsLive)
{
    mStatus.hasFix       = true;
    mStatus.fix          = fix;
    mStatus.precisionM   = precisionM;
    mStatus.headingValid = mCourse.update(fix);
    mStatus.headingDeg   = mCourse.headingDeg();

    if (!mStatus.routeLoaded) {
        return OffCourse::Event::None;
    }
    mStatus.pos      = mTracker.update(fix);
    mStatus.toStartM = Geo::distanceM(fix, mPoints[0]);

    // The alert only runs during an activity: frozen on the start screen and
    // while paused, so neither can raise one.
    OffCourse::Event event = OffCourse::Event::None;
    if (alertsLive) {
        event = mOffCourse.update(nowMs, mStatus.pos, precisionM);
    }
    mStatus.alert   = mOffCourse.state();
    mStatus.offForS = mOffCourse.offForMs(nowMs) / 1000u;
    return event;
}

} // namespace Trail
