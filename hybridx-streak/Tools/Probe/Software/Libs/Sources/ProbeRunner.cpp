/**
 ******************************************************************************
 * @file    ProbeRunner.cpp
 * @brief   The Streak Probe's checks (see the header).
 ******************************************************************************
 */

#include "ProbeRunner.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

using SDK::Interface::IFileSystem;

namespace Probe
{

namespace
{

char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

bool endsWithFit(const char* name, size_t len)
{
    return len >= 4 && name[len - 4] == '.' && lower(name[len - 3]) == 'f' && lower(name[len - 2]) == 'i'
           && lower(name[len - 1]) == 't';
}

/// snprintf into a path buffer; false (and the path unusable) if it would not fit.
bool makePath(char* out, size_t n, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
bool makePath(char* out, size_t n, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int len = vsnprintf(out, n, fmt, args);
    va_end(args);
    return len > 0 && static_cast<size_t>(len) < n;
}

void copyText(char* out, size_t n, const char* text)
{
    if (n == 0) {
        return;
    }
    std::strncpy(out, text, n - 1);
    out[n - 1] = '\0';
}

} // namespace

bool isFit(const char* name)
{
    return name && endsWithFit(name, std::strlen(name));
}

bool isActivityFit(const char* name)
{
    return name && std::strncmp(name, "activity_", 9) == 0 && isFit(name);
}

bool isMonth(const char* name)
{
    if (!name || std::strlen(name) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) {
        if (name[i] < '0' || name[i] > '9') {
            return false;
        }
    }
    return true;
}

Runner::Runner(IFileSystem& fs, Host& host)
    : mFs(fs)
    , mHost(host)
{
}

void Runner::say(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(mLine, sizeof(mLine), fmt, args);
    va_end(args);
    mHost.line(mLine);
}

int Runner::listCount(const char* path)
{
    auto dir = mFs.dir(path);
    if (!dir || !dir->open()) {
        return -1;
    }
    int n = 0;
    while (n < kMaxEntries && dir->readNext(mInfo)) {
        ++n;
    }
    dir->close();
    return n;
}

void Runner::run(Result& r)
{
    r = Result {};
    mAppCount      = 0;
    mCandidate[0]  = '\0';
    mCandidateName[0] = '\0';
    mLargest[0]    = '\0';
    mLargestSize   = 0;

    // [1] Routes out of the sandbox --------------------------------------------
    say("[1] Listing folders");
    struct Route {
        const char* path;
        Check*      check;
    };
    const Route routes[] = {
        { "..", &r.listParent },
        { "/Apps", &r.listApps },
        { "2:/Apps", &r.listDriveApps },
        { "/", &r.listRoot },
    };
    const char* base = nullptr;
    for (const Route& route : routes) {
        const int n = listCount(route.path);
        *route.check = n >= 0 ? Check::Ok : Check::Failed;
        if (n >= 0) {
            say("  list %-8s ok, %d entries", route.path, n);
        } else {
            say("  list %-8s FAIL (cannot open)", route.path);
        }
        // "/" lists the volume, not the apps, so it is never the scan base.
        if (!base && n > 0 && route.path[1] != '\0') {
            base = route.path;
        }
    }
    const int own = listCount(".");
    say("  list %-8s %s, %d entries", ".", own >= 0 ? "ok" : "FAIL", own);

    if (!base) {
        r.verdict = Verdict::Blocked;
        say("  No route out of this app's folder lists anything.");
    } else {
        copyText(r.base, sizeof(r.base), base);
        say("  Scanning apps under \"%s\"", base);

        // [2] Apps and their activity files --------------------------------------
        say("[2] Apps and activity files");
        collectApps(base);
        r.apps = mAppCount;
        for (uint8_t i = 0; i < mAppCount; ++i) {
            scanApp(base, mApps[i], r);
        }
        say("  %u apps, %u with .fit; %u activity_*.fit, %u other .fit, %u .recording",
            static_cast<unsigned>(r.apps), static_cast<unsigned>(r.appsWithFit), static_cast<unsigned>(r.fitFiles),
            static_cast<unsigned>(r.otherFit), static_cast<unsigned>(r.recordingMarks));

        // [3] Open the newest, and time the largest ----------------------------
        say("[3] Opening a .fit");
        openCandidate(r);
        readLargest(r);

        if (r.fitFiles == 0 && r.otherFit == 0) {
            r.verdict = Verdict::NoFiles;
        } else if (r.fitOpen == Check::Ok && r.fitSignature == Check::Ok) {
            r.verdict = Verdict::Go;
        } else {
            r.verdict = Verdict::NoOpen;
        }
    }

    // [5] and [6] run whatever the verdict: they matter to the design either way.
    say("[5] SharedData");
    sharedData(r);
    say("[6] Rename onto an existing file");
    renameOver(r);

    // [7] Exact paths, no listing ----------------------------------------------
    say("[7] Other apps' summary.json, opened by name");
    byName(r);

    // On the watch, listing ".." reached another volume while files opened by
    // name reached /Apps (NOTES, Gate 0), so [7] outranks the listing's answer.
    // A listed .fit that opened (GO) or refused (NO READ) is a concrete answer
    // and stands; otherwise [7] decides.
    if (r.summariesRead > 0) {
        r.verdict = Verdict::Go;
    } else if (r.verdict != Verdict::Go && r.verdict != Verdict::NoOpen) {
        if (r.summariesFound > 0) {
            r.verdict = Verdict::NoOpen;
        } else if (r.appsSeen > 0) {
            r.verdict = Verdict::NoFiles;
        } else {
            r.verdict = Verdict::Blocked;
        }
    }

    say("Verdict: %s", verdictName(r.verdict));
}

void Runner::collectApps(const char* base)
{
    auto dir = mFs.dir(base);
    if (!dir || !dir->open()) {
        return;
    }
    uint16_t seen = 0;
    while (seen < kMaxEntries && dir->readNext(mInfo)) {
        ++seen;
        if (!mInfo.isDir || mInfo.name[0] == '.' || std::strcmp(mInfo.name, "SharedData") == 0) {
            continue;
        }
        if (std::strlen(mInfo.name) >= kMaxAppName) {
            say("  (skipped long name %.40s)", mInfo.name);
            continue;
        }
        if (mAppCount >= kMaxApps) {
            say("  (more than %u apps; the rest are not scanned)", static_cast<unsigned>(kMaxApps));
            break;
        }
        copyText(mApps[mAppCount++], kMaxAppName, mInfo.name);
    }
    dir->close();
}

void Runner::scanApp(const char* base, const char* app, Result& r)
{
    char activity[IFileSystem::skMaxPathLen];
    if (!makePath(activity, sizeof(activity), "%s/%s/Activity", base, app)) {
        return;
    }

    // Pass 1: the Activity folder itself. Month folders are collected first so
    // that no two directories are ever open at once.
    auto dir = mFs.dir(activity);
    if (!dir || !dir->open()) {
        say("  %-16s no Activity folder", app);
        return;
    }
    uint8_t  months    = 0;
    uint16_t fitBefore = r.fitFiles + r.otherFit;
    bool     marker    = false;
    uint16_t seen      = 0;
    while (seen < kMaxEntries && dir->readNext(mInfo)) {
        ++seen;
        if (mInfo.isDir && isMonth(mInfo.name)) {
            if (months < kMaxMonths) {
                copyText(mMonths[months++], sizeof(mMonths[0]), mInfo.name);
            }
        } else if (!mInfo.isDir && std::strcmp(mInfo.name, ".recording") == 0) {
            marker = true;
        } else if (!mInfo.isDir && isFit(mInfo.name)) {
            if (makePath(mPath, sizeof(mPath), "%s/%s", activity, mInfo.name)) {
                consider(mPath, app, "", mInfo.name, mInfo.size, false, r);
            }
        }
    }
    dir->close();

    // Pass 2: each month folder.
    for (uint8_t m = 0; m < months; ++m) {
        char monthPath[IFileSystem::skMaxPathLen];
        if (!makePath(monthPath, sizeof(monthPath), "%s/%s", activity, mMonths[m])) {
            continue;
        }
        auto month = mFs.dir(monthPath);
        if (!month || !month->open()) {
            say("  %-16s cannot open %s", app, mMonths[m]);
            continue;
        }
        uint16_t count = 0;
        while (count < kMaxEntries && month->readNext(mInfo)) {
            ++count;
            if (mInfo.isDir || !isFit(mInfo.name)) {
                continue;
            }
            if (makePath(mPath, sizeof(mPath), "%s/%s", monthPath, mInfo.name)) {
                consider(mPath, app, mMonths[m], mInfo.name, mInfo.size, isActivityFit(mInfo.name), r);
            }
        }
        month->close();
    }

    const uint16_t found = static_cast<uint16_t>(r.fitFiles + r.otherFit - fitBefore);
    if (found > 0) {
        ++r.appsWithFit;
    }
    if (marker) {
        ++r.recordingMarks;
    }
    say("  %-16s %u months, %u .fit%s", app, static_cast<unsigned>(months), static_cast<unsigned>(found),
        marker ? ", .recording present" : "");
}

void Runner::consider(const char* path, const char* app, const char* month, const char* name, size_t size,
                      bool standardName, Result& r)
{
    if (standardName) {
        ++r.fitFiles;
    } else {
        ++r.otherFit;
    }

    // The newest standard file wins, compared by the local start time in its
    // name; failing any, the first other .fit found stands in.
    const bool better = standardName
                            ? (!isActivityFit(mCandidateName) || std::strcmp(name, mCandidateName) > 0)
                            : mCandidate[0] == '\0';
    if (better) {
        copyText(mCandidate, sizeof(mCandidate), path);
        copyText(mCandidateName, sizeof(mCandidateName), name);
        if (month[0] != '\0') {
            snprintf(r.newest, sizeof(r.newest), "%s/%s/%s", app, month, name);
        } else {
            snprintf(r.newest, sizeof(r.newest), "%s/%s", app, name);
        }
    }
    if (size > mLargestSize || mLargest[0] == '\0') {
        mLargestSize = size;
        copyText(mLargest, sizeof(mLargest), path);
    }
}

void Runner::openCandidate(Result& r)
{
    if (mCandidate[0] == '\0') {
        say("  no .fit found to open");
        return;
    }
    auto file = mFs.file(mCandidate);
    if (!file || !file->open(false, false)) {
        r.fitOpen = Check::Failed;
        say("  open FAIL: %.120s", mCandidate);
        return;
    }
    r.fitOpen = Check::Ok;

    // A FIT file header: byte 0 its size (12 or 14), bytes 8..11 ".FIT".
    unsigned char head[12] = {};
    size_t        got      = 0;
    const bool    ok       = file->read(reinterpret_cast<char*>(head), sizeof(head), got);
    const size_t  size     = file->size();
    file->close();

    r.fitSignature = (ok && got == sizeof(head) && std::memcmp(head + 8, ".FIT", 4) == 0) ? Check::Ok : Check::Failed;
    say("  open ok: %.120s", mCandidate);
    say("  %u bytes, header %u, signature %s", static_cast<unsigned>(size), static_cast<unsigned>(head[0]),
        r.fitSignature == Check::Ok ? ".FIT ok" : "NOT FIT");
}

void Runner::readLargest(Result& r)
{
    if (mLargest[0] == '\0') {
        return;
    }
    auto file = mFs.file(mLargest);
    if (!file || !file->open(false, false)) {
        say("  read test: cannot open the largest file");
        return;
    }
    const uint32_t start = mHost.nowMs();
    uint32_t       total = 0;
    size_t         got   = 0;
    while (total < kMaxReadBytes && file->read(mBuf, sizeof(mBuf), got) && got > 0) {
        total += static_cast<uint32_t>(got);
    }
    // Unsigned subtraction stays right across the clock's wrap.
    const uint32_t ms = mHost.nowMs() - start;
    file->close();

    r.readBytes = total;
    r.readMs    = ms;
    const uint32_t kbPerS = ms > 0 ? static_cast<uint32_t>((static_cast<uint64_t>(total) * 1000u) / (ms * 1024u)) : 0u;
    say("  read %u bytes in %u ms (%u KB/s)%s", static_cast<unsigned>(total), static_cast<unsigned>(ms),
        static_cast<unsigned>(kbPerS), total >= kMaxReadBytes ? ", capped" : "");
}

bool Runner::writeText(const char* path, const char* text)
{
    auto file = mFs.file(path);
    if (!file || !file->open(true, true)) {
        return false;
    }
    const size_t len     = std::strlen(text);
    size_t       written = 0;
    const bool   ok      = file->write(text, len, written) && written == len;
    file->close();
    return ok;
}

bool Runner::readText(const char* path, char* out, size_t n)
{
    out[0]    = '\0';
    auto file = mFs.file(path);
    if (!file || !file->open(false, false)) {
        return false;
    }
    size_t     got = 0;
    const bool ok  = file->read(out, n - 1, got);
    file->close();
    out[ok ? got : 0] = '\0';
    return ok;
}

void Runner::sharedData(Result& r)
{
    static constexpr const char* kText = "HybridX Streak probe";
    const bool made    = mFs.mkdir(kSharedDir);
    const bool wrote   = made && writeText(kSharedTmp, kText);
    const bool read    = wrote && readText(kSharedTmp, mBuf, sizeof(mBuf)) && std::strcmp(mBuf, kText) == 0;
    const bool removed = wrote && mFs.remove(kSharedTmp);
    r.sharedData       = (made && wrote && read && removed) ? Check::Ok : Check::Failed;
    say("  mkdir %s, write %s, read back %s, remove %s", made ? "ok" : "FAIL", wrote ? "ok" : "FAIL",
        read ? "ok" : "FAIL", removed ? "ok" : "FAIL");
}

void Runner::renameOver(Result& r)
{
    if (!writeText(kRenameFrom, "A") || !writeText(kRenameTo, "B")) {
        r.renameRefused = Check::NotRun;
        say("  could not write the test files in this app's folder");
    } else {
        const bool renamed = mFs.rename(kRenameFrom, kRenameTo);
        readText(kRenameTo, mBuf, sizeof(mBuf));
        if (!renamed && std::strcmp(mBuf, "B") == 0) {
            r.renameRefused = Check::Ok;
            say("  refused, destination kept (as FatFs): save must remove, then rename");
        } else {
            r.renameRefused = Check::Failed;
            say("  rename %s, destination now \"%.8s\"", renamed ? "succeeded" : "failed", mBuf);
        }
    }
    mFs.remove(kRenameFrom);
    mFs.remove(kRenameTo);
}

// -- [7] Exact paths ------------------------------------------------------------------

bool Runner::jsonNumber(const char* text, const char* key, uint32_t& out)
{
    if (!text || !key) {
        return false;
    }
    char   want[24];
    const int n = snprintf(want, sizeof(want), "\"%s\"", key);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(want)) {
        return false;
    }
    const char* at = std::strstr(text, want);
    if (!at) {
        return false;
    }
    at += n;
    while (*at == ' ' || *at == '\t' || *at == '\n' || *at == '\r') {
        ++at;
    }
    if (*at != ':') {
        return false;
    }
    ++at;
    while (*at == ' ' || *at == '\t' || *at == '\n' || *at == '\r') {
        ++at;
    }
    if (*at < '0' || *at > '9') {
        return false;   // negative, a string, or missing: not what a summary holds
    }
    uint64_t v = 0;
    while (*at >= '0' && *at <= '9') {
        v = v * 10u + static_cast<uint64_t>(*at - '0');
        if (v > 0xFFFFFFFFull) {
            return false;
        }
        ++at;
    }
    out = static_cast<uint32_t>(v);
    return true;
}

void Runner::monthOf(uint32_t utc, char out[7])
{
    // Civil-from-days (Howard Hinnant), for days since 1970-01-01.
    const int64_t  z   = static_cast<int64_t>(utc / 86400u) + 719468;
    const int64_t  era = z / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp  = (5 * doy + 2) / 153;
    const uint32_t m   = mp < 10 ? mp + 3 : mp - 9;
    const int64_t  y   = static_cast<int64_t>(yoe) + era * 400 + (m <= 2 ? 1 : 0);
    const uint32_t yy = static_cast<uint32_t>(y < 0 ? 0 : y % 10000);
    out[0] = static_cast<char>('0' + yy / 1000);
    out[1] = static_cast<char>('0' + yy / 100 % 10);
    out[2] = static_cast<char>('0' + yy / 10 % 10);
    out[3] = static_cast<char>('0' + yy % 10);
    out[4] = static_cast<char>('0' + m / 10);
    out[5] = static_cast<char>('0' + m % 10);
    out[6] = '\0';
}

void Runner::addName(const char* name, size_t len)
{
    while (len > 0 && (name[0] == ' ' || name[0] == '\t')) {
        ++name;
        --len;
    }
    while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '\t' || name[len - 1] == '\r')) {
        --len;
    }
    if (len == 0 || len >= kMaxAppName || name[0] == '#') {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        if (name[i] == '/' || name[i] == '\\' || name[i] == ':' || (name[i] == '.' && i + 1 < len && name[i + 1] == '.')) {
            return;   // a folder name, never a path
        }
    }
    if (len == std::strlen(kOwnApp) && std::strncmp(name, kOwnApp, len) == 0) {
        return;
    }
    for (uint8_t i = 0; i < mNameCount; ++i) {
        if (std::strlen(mNames[i]) == len && std::strncmp(mNames[i], name, len) == 0) {
            return;
        }
    }
    if (mNameCount >= kMaxNames) {
        return;
    }
    std::memcpy(mNames[mNameCount], name, len);
    mNames[mNameCount][len] = '\0';
    ++mNameCount;
}

void Runner::loadNames()
{
    mNameCount = 0;
    for (const char* known : kKnownApps) {
        addName(known, std::strlen(known));
    }
    const uint8_t builtIn = mNameCount;
    if (readText(kAppsFile, mBuf, sizeof(mBuf))) {
        const char* start = mBuf;
        for (const char* p = mBuf;; ++p) {
            if (*p == '\n' || *p == ',' || *p == '\0') {
                addName(start, static_cast<size_t>(p - start));
                start = p + 1;
            }
            if (*p == '\0') {
                break;
            }
        }
        say("  %s: %u more name(s)", kAppsFile, static_cast<unsigned>(mNameCount - builtIn));
    } else {
        say("  no %s: built-in names only (add one name per line to try more)", kAppsFile);
    }
}

void Runner::byName(Result& r)
{
    // Controls first: a file this probe knows is there, reached through "..".
    if (mFs.exist(kSharedFile)) {
        r.sharedByName = readText(kSharedFile, mBuf, sizeof(mBuf)) && mBuf[0] == '{' ? Check::Ok : Check::Failed;
        say("  control: %s %s", kSharedFile, r.sharedByName == Check::Ok ? "opens" : "exists, OPEN FAIL");
    } else {
        say("  control: %s not there (open HybridX Streak once to create it)", kSharedFile);
    }
    char own[64];
    if (makePath(own, sizeof(own), "../%s", kOwnApp)) {   // this probe's own folder, seen from outside
        say("  control: %s %s", own, mFs.exist(own) ? "exists" : "not seen");
    }

    loadNames();
    for (uint8_t i = 0; i < mNameCount; ++i) {
        trySummary(mNames[i], r);
    }
    r.namesTried = mNameCount;
    say("  %u names, %u folders seen, %u summaries found, %u read, %u .fit in their month",
        static_cast<unsigned>(r.namesTried), static_cast<unsigned>(r.appsSeen),
        static_cast<unsigned>(r.summariesFound), static_cast<unsigned>(r.summariesRead),
        static_cast<unsigned>(r.fitByName));
}

void Runner::trySummary(const char* app, Result& r)
{
    static constexpr const char* kRoutes[] = { "..", "/Apps", "2:/Apps" };

    const bool seen = makePath(mPath, sizeof(mPath), "../%s", app) && mFs.exist(mPath);
    if (seen) {
        ++r.appsSeen;
    }

    bool found = false;
    for (const char* route : kRoutes) {
        if (!makePath(mPath, sizeof(mPath), "%s/%s/Activity/summary.json", route, app)) {
            continue;
        }
        const bool exists = mFs.exist(mPath);
        // Open even when exist() says no: on the watch, what one call refuses
        // another may allow, and trying costs nothing.
        const bool read = readText(mPath, mBuf, sizeof(mBuf));
        if (!exists && !read) {
            continue;
        }
        if (!found) {
            ++r.summariesFound;
            found = true;
        }
        uint32_t utc = 0;
        uint32_t secs = 0;
        uint32_t metres = 0;
        const bool parsed = read && jsonNumber(mBuf, "utc", utc);
        if (!parsed) {
            say("  %-14s %s: %s", app, route, read ? "opens, but no \"utc\"" : "exists, OPEN FAIL");
            continue;
        }
        jsonNumber(mBuf, "time", secs);
        jsonNumber(mBuf, "distance", metres);
        ++r.summariesRead;
        say("  %-14s %s: read %u B, utc %lu, %lu s, %lu m", app, route, static_cast<unsigned>(std::strlen(mBuf)),
            static_cast<unsigned long>(utc), static_cast<unsigned long>(secs), static_cast<unsigned long>(metres));
        if (utc >= r.summaryUtc) {
            copyText(r.summaryApp, sizeof(r.summaryApp), app);
            copyText(r.summaryRoute, sizeof(r.summaryRoute), route);
            r.summaryUtc    = utc;
            r.summarySecs   = secs;
            r.summaryMetres = metres;
        }
        tryMonth(route, app, utc, r);
        return;   // one route that works is enough
    }
    if (!found) {
        say("  %-14s %s", app, seen ? "folder seen, no summary.json by any route" : "-");
    }
}

void Runner::tryMonth(const char* route, const char* app, uint32_t utc, Result& r)
{
    // The activity's own .fit, if the phone hasn't synced it away yet. The
    // month comes from the summary's UTC time, so one recorded just after a
    // local midnight at a month's end may be missed: fine for a probe.
    char month[7];
    monthOf(utc, month);
    char folder[IFileSystem::skMaxPathLen];
    if (!makePath(folder, sizeof(folder), "%s/%s/Activity/%s", route, app, month)) {
        return;
    }
    auto dir = mFs.dir(folder);
    if (!dir || !dir->open()) {
        say("  %-14s   %s/: cannot list", app, month);
        return;
    }
    uint16_t seen  = 0;
    uint16_t fits  = 0;
    mCandidate[0]  = '\0';
    while (seen < kMaxEntries && dir->readNext(mInfo)) {
        ++seen;
        if (!mInfo.isDir && isFit(mInfo.name)) {
            ++fits;
            if (mCandidate[0] == '\0') {
                makePath(mCandidate, sizeof(mCandidate), "%s/%s", folder, mInfo.name);
            }
        }
    }
    dir->close();
    r.fitByName = static_cast<uint8_t>(r.fitByName + (fits > 255 - r.fitByName ? 255 - r.fitByName : fits));

    if (mCandidate[0] == '\0') {
        say("  %-14s   %s/: %u entries, no .fit (synced away?)", app, month, static_cast<unsigned>(seen));
        return;
    }
    auto file = mFs.file(mCandidate);
    unsigned char head[12] = {};
    size_t        got      = 0;
    const bool    opened   = file && file->open(false, false);
    const bool    ok       = opened && file->read(reinterpret_cast<char*>(head), sizeof(head), got) && got == sizeof(head)
                    && std::memcmp(head + 8, ".FIT", 4) == 0;
    if (opened) {
        file->close();
    }
    if (r.fitByNameOpen != Check::Ok) {
        r.fitByNameOpen = ok ? Check::Ok : Check::Failed;
    }
    say("  %-14s   %s/: %u .fit, first %s", app, month, static_cast<unsigned>(fits),
        ok ? "opens (.FIT ok)" : (opened ? "opens, NOT FIT" : "OPEN FAIL"));
}

size_t Runner::historyLine(const Result& r, const char* stamp, char* out, size_t n)
{
    if (n == 0) {
        return 0;
    }
    const int len = snprintf(out, n, "run %u | %s | %s | base %s | apps %u, with fit %u | fit %u (+%u) | newest %s | "
                                     "read %u B %u ms | shared %s | rename %s | glance %dx%d %u | by name %u/%u/%u/%u, fit %u %s, "
                                     "newest %s %s utc %lu %lus",
                             static_cast<unsigned>(r.run), stamp ? stamp : "?", verdictName(r.verdict),
                             r.base[0] ? r.base : "-", static_cast<unsigned>(r.apps),
                             static_cast<unsigned>(r.appsWithFit), static_cast<unsigned>(r.fitFiles),
                             static_cast<unsigned>(r.otherFit), r.newest[0] ? r.newest : "-",
                             static_cast<unsigned>(r.readBytes), static_cast<unsigned>(r.readMs),
                             checkName(r.sharedData), renameName(r.renameRefused), r.glanceWidth, r.glanceHeight,
                             static_cast<unsigned>(r.glanceControls), static_cast<unsigned>(r.namesTried),
                             static_cast<unsigned>(r.appsSeen), static_cast<unsigned>(r.summariesFound),
                             static_cast<unsigned>(r.summariesRead), static_cast<unsigned>(r.fitByName),
                             checkName(r.fitByNameOpen), r.summaryApp[0] ? r.summaryApp : "-",
                             r.summaryRoute[0] ? r.summaryRoute : "-", static_cast<unsigned long>(r.summaryUtc),
                             static_cast<unsigned long>(r.summarySecs));
    if (len < 0) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(len) < n ? static_cast<size_t>(len) : n - 1;
}

} // namespace Probe
