/**
 ******************************************************************************
 * @file    ProbeRunner.cpp
 * @brief   The Intervals Probe's check (see the header).
 ******************************************************************************
 */

#include "ProbeRunner.hpp"

#include <cstdio>
#include <cstring>

using SDK::Interface::IFileSystem;

namespace Probe
{

Runner::Runner(IFileSystem& fs, Host& host)
    : mFs(fs)
    , mHost(host)
{
}

void Runner::sanitise(const char* in, size_t inLen, char* out, size_t outCap)
{
    size_t n = inLen < outCap - 1 ? inLen : outCap - 1;
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        // Printable ASCII and plain whitespace only: a non-text file (or a
        // torn write) must never garble the watch screen or the report.
        out[i] = (c >= 0x20 && c < 0x7F) || c == '\n' || c == '\t' ? static_cast<char>(c) : '.';
    }
    out[n] = '\0';
}

void Runner::run(Result& r)
{
    r = Result {};

    const bool made = mFs.mkdir(Runner::kPlansDir);
    auto       dir   = mFs.dir(Runner::kPlansDir);
    const bool open  = made && dir && dir->open();
    r.folderOk        = open ? Check::Ok : Check::Failed;
    if (!open) {
        r.verdict = Verdict::FolderFailed;
        snprintf(mLine, sizeof(mLine), "Plans/ %s, %s", made ? "created/existed" : "mkdir FAILED",
                 dir ? "open FAILED" : "no directory object");
        mHost.line(mLine);
        return;
    }

    uint16_t seen = 0;
    while (seen < kMaxEntries && dir->readNext(mInfo)) {
        ++seen;
        if (mInfo.isDir) {
            continue;
        }
        ++r.fileCount;
        if (r.fileCount == 1 || mInfo.utc >= r.newestUtc) {
            std::strncpy(r.newestName, mInfo.name, sizeof(r.newestName) - 1);
            r.newestName[sizeof(r.newestName) - 1] = '\0';
            r.newestSize                           = static_cast<uint32_t>(mInfo.size);
            r.newestUtc                            = mInfo.utc;
        }
    }
    dir->close();
    snprintf(mLine, sizeof(mLine), "Plans/: %u file(s)", static_cast<unsigned>(r.fileCount));
    mHost.line(mLine);

    if (r.fileCount == 0) {
        r.verdict = Verdict::Empty;
        mHost.line("  nothing there yet: the write has not arrived (or hasn't been sent)");
        return;
    }

    snprintf(mPath, sizeof(mPath), "%s/%s", Runner::kPlansDir, r.newestName);
    auto file = mFs.file(mPath);
    if (!file || !file->open(false, false)) {
        r.verdict = Verdict::Empty;   // found an entry, but couldn't read it: treat as not yet usable
        snprintf(mLine, sizeof(mLine), "  newest \"%s\", %u bytes -- OPEN FAILED", r.newestName,
                 static_cast<unsigned>(r.newestSize));
        mHost.line(mLine);
        return;
    }
    size_t got = 0;
    file->read(mBuf, sizeof(mBuf) - 1, got);
    file->close();
    sanitise(mBuf, got, r.preview, sizeof(r.preview));

    r.verdict = Verdict::Go;
    snprintf(mLine, sizeof(mLine), "  newest \"%s\", %u bytes, read %u: %s", r.newestName,
             static_cast<unsigned>(r.newestSize), static_cast<unsigned>(got), r.preview);
    mHost.line(mLine);
}

} // namespace Probe
