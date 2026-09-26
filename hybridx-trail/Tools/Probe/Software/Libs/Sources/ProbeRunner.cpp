/**
 ******************************************************************************
 * @file    ProbeRunner.cpp
 * @brief   The Trail Probe's route check (see the header).
 ******************************************************************************
 */

#include "ProbeRunner.hpp"

#include <cstdio>
#include <cstring>

using SDK::Interface::IFileSystem;

namespace Probe
{

Runner::Runner(IFileSystem& fs, Host& host, Trail::RouteBuilder& route)
    : mFs(fs)
    , mHost(host)
    , mRoute(route)
    , mReader(route)
{
}

bool Runner::isRouteFile(const char* name)
{
    const size_t n = std::strlen(name);
    if (n < 5 || name[0] == '.') {
        return false;
    }
    const char* ext = name + n - 4;
    return ext[0] == '.' && (ext[1] | 0x20) == 'g' && (ext[2] | 0x20) == 'p' && (ext[3] | 0x20) == 'x';
}

void Runner::asciiFold(const char* in, char* out, size_t outCap)
{
    size_t o = 0;
    for (size_t i = 0; in[i] != '\0' && o + 1 < outCap; ++i) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        if (c >= 0x80) {
            if ((c & 0xC0) != 0x80) {   // the lead byte of a sequence; continuation bytes vanish
                out[o++] = '?';
            }
        } else {
            out[o++] = c >= 0x20 && c < 0x7F ? static_cast<char>(c) : '.';
        }
    }
    out[o] = '\0';
}

void Runner::run(Result& r)
{
    const bool made = mFs.mkdir(kRoutesDir);
    auto       dir  = mFs.dir(kRoutesDir);
    const bool open = made && dir && dir->open();
    if (!open) {
        r.verdict = Verdict::FolderFailed;
        snprintf(mLine, sizeof(mLine), "Routes/ %s, %s", made ? "created/existed" : "mkdir FAILED",
                 dir ? "open FAILED" : "no directory object");
        mHost.line(mLine);
        return;
    }

    uint16_t seen       = 0;
    time_t   newestUtc  = 0;
    mNewest[0]          = '\0';
    while (seen < kMaxEntries && dir->readNext(mInfo)) {
        ++seen;
        if (mInfo.isDir || !isRouteFile(mInfo.name)) {
            ++r.skipped;
            snprintf(mLine, sizeof(mLine), "  skipped \"%.60s\"%s", mInfo.name, mInfo.isDir ? " (folder)" : "");
            mHost.line(mLine);
            continue;
        }
        ++r.gpxCount;
        snprintf(mLine, sizeof(mLine), "  found \"%.60s\", %lu bytes, utc %lld", mInfo.name,
                 static_cast<unsigned long>(mInfo.size), static_cast<long long>(mInfo.utc));
        mHost.line(mLine);
        if (r.gpxCount == 1 || mInfo.utc >= newestUtc) {
            std::strncpy(mNewest, mInfo.name, sizeof(mNewest) - 1);
            mNewest[sizeof(mNewest) - 1] = '\0';
            newestUtc                    = mInfo.utc;
            r.fileBytes                  = static_cast<uint32_t>(mInfo.size);
        }
    }
    dir->close();
    snprintf(mLine, sizeof(mLine), "Routes/: %u .gpx file(s), %u other entr%s", static_cast<unsigned>(r.gpxCount),
             static_cast<unsigned>(r.skipped), r.skipped == 1 ? "y" : "ies");
    mHost.line(mLine);

    if (r.gpxCount == 0) {
        r.verdict = Verdict::NoRoute;
        mHost.line("  no route yet: copy a .gpx into Apps/HXTrailProbe/Routes/ over USB");
        return;
    }
    asciiFold(mNewest, r.file, sizeof(r.file));

    snprintf(mPath, sizeof(mPath), "%s/%.240s", kRoutesDir, mNewest);
    auto file = mFs.file(mPath);
    if (!file || !file->open(false, false)) {
        r.verdict = Verdict::Unreadable;
        snprintf(mLine, sizeof(mLine), "  newest \"%s\": OPEN FAILED", r.file);
        mHost.line(mLine);
        return;
    }

    mReader.reset();
    mRoute.reset();
    size_t got = 0;
    while (r.bytesRead < kMaxBytes && file->read(mChunk, sizeof(mChunk), got) && got > 0) {
        mReader.feed(mChunk, got);
        r.bytesRead += static_cast<uint32_t>(got);
    }
    file->close();
    mRoute.finish();

    const Trail::GpxReader::Stats& s = mReader.stats();
    r.looksGpx  = s.sawGpx;
    r.rawPoints = mRoute.rawPoints();
    r.badPoints = s.badPoints;
    r.ignored   = mRoute.ignoredPoints();
    r.kept      = mRoute.count();
    r.spacingM  = mRoute.spacingM();
    r.lengthM   = mRoute.lengthM();
    r.hasEle    = mRoute.hasElevation();
    r.ascentM   = mRoute.ascentM();
    asciiFold(s.name, r.name, sizeof(r.name));

    snprintf(mLine, sizeof(mLine), "  newest \"%s\": read %lu of %lu bytes, <gpx> %s", r.file,
             static_cast<unsigned long>(r.bytesRead), static_cast<unsigned long>(r.fileBytes),
             s.sawGpx ? "seen" : "NOT seen");
    mHost.line(mLine);
    snprintf(mLine, sizeof(mLine), "  name \"%s\"; trkpt %lu, rtept %lu, wpt %lu, bad %lu, long tags %lu",
             r.name, static_cast<unsigned long>(s.trackPoints), static_cast<unsigned long>(s.routePoints),
             static_cast<unsigned long>(s.waypoints), static_cast<unsigned long>(s.badPoints),
             static_cast<unsigned long>(s.longTags));
    mHost.line(mLine);
    snprintf(mLine, sizeof(mLine), "  route: %lu m, %u of %lu points kept at %u m spacing, ascent %s%lu m",
             static_cast<unsigned long>(r.lengthM), static_cast<unsigned>(r.kept),
             static_cast<unsigned long>(r.rawPoints), static_cast<unsigned>(r.spacingM), r.hasEle ? "" : "(no ele) ",
             static_cast<unsigned long>(r.ascentM));
    mHost.line(mLine);

    r.verdict = r.kept >= 2 ? Verdict::Go : Verdict::Unreadable;
}

} // namespace Probe
