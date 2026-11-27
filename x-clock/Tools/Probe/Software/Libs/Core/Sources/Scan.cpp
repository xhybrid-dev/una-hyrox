/**
 ******************************************************************************
 * @file    Scan.cpp
 * @brief   See the header.
 ******************************************************************************
 */

#include "Scan.hpp"

#include <cstring>

namespace ClockProbe
{

const char* verdictName(Verdict v)
{
    switch (v) {
        case Verdict::NotThere:   return "not there";
        case Verdict::CannotRead: return "cannot read";
        case Verdict::Parsed:     return "parsed";
        case Verdict::BadFormat:  return "bad format";
        case Verdict::SizeOnly:   return "size only";
    }
    return "?";
}

void ScanData::clear()
{
    // One member at a time: the biggest temporary is X Training's file, about a kilobyte.
    for (ScanFile& f : files) {
        f = ScanFile {};
    }
    shared  = ListResult {};
    hybridx = ListResult {};
    for (uint8_t i = 0; i < kLinkFiles; ++i) {
        act[i]   = HybridXLink::ActivityFile {};
        actOk[i] = false;
    }
    streakNow   = HybridXLink::StreakNow {};
    streakNowOk = false;
    today       = XTraining::TodayData {};
    todayOk     = false;
    vo2         = RunVo2::Glance::View {};
    vo2Ok       = false;
}

uint8_t ScanData::found() const
{
    uint8_t n = 0;
    for (const ScanFile& f : files) {
        if (f.r.exists) {
            ++n;
        }
    }
    return n;
}

uint8_t ScanData::problems() const
{
    uint8_t n = 0;
    for (const ScanFile& f : files) {
        if (f.r.exists && (f.verdict == Verdict::CannotRead || f.verdict == Verdict::BadFormat)) {
            ++n;
        }
    }
    return n;
}

namespace
{
struct ListCtx {
    const ScanSink* sink;
    const char*     dir;
};

void onListed(void* ctx, const ListEntry& e)
{
    const ListCtx* c = static_cast<const ListCtx*>(ctx);
    if (c->sink->listed != nullptr) {
        c->sink->listed(c->sink->ctx, c->dir, e);
    }
}

Verdict parse(Role role, uint8_t index, const char* text, size_t len, ScanData& out)
{
    switch (role) {
        case Role::Link: {
            if (index >= kLinkFiles) {
                return Verdict::BadFormat;
            }
            const bool ok = HybridXLink::decode(text, len, out.act[index]);
            out.actOk[index] = ok;
            return ok ? Verdict::Parsed : Verdict::BadFormat;
        }
        case Role::StreakNow:
            out.streakNowOk = HybridXLink::decodeStreakNow(text, len, out.streakNow);
            return out.streakNowOk ? Verdict::Parsed : Verdict::BadFormat;
        case Role::TrainingToday:
            out.todayOk = XTraining::TodayFile::read(text, len, out.today);
            return out.todayOk ? Verdict::Parsed : Verdict::BadFormat;
        case Role::Vo2Max:
            out.vo2Ok = RunVo2::Glance::makeView(text, len, out.vo2);
            return out.vo2Ok ? Verdict::Parsed : Verdict::BadFormat;
        case Role::SizeOnly:
            break;
    }
    return Verdict::SizeOnly;
}
} // namespace

void runScan(SDK::Interface::IFileSystem& fs, char* scratch, size_t cap, ScanData& out, const ScanSink& sink)
{
    out.clear();

    ListCtx lc {&sink, kSharedDir};
    out.shared = listDir(fs, kSharedDir, &onListed, &lc);
    if (sink.listDone != nullptr) {
        sink.listDone(sink.ctx, kSharedDir, out.shared);
    }
    lc.dir      = kHybridxDir;
    out.hybridx = listDir(fs, kHybridxDir, &onListed, &lc);
    if (sink.listDone != nullptr) {
        sink.listDone(sink.ctx, kHybridxDir, out.hybridx);
    }

    for (uint8_t i = 0; i < kKnownCount; ++i) {
        const Known& k = kKnown[i];
        const bool   read = (k.role != Role::SizeOnly);
        ScanFile&    f = out.files[i];
        f.r = probeFile(fs, k.path, scratch, cap, read);
        const char* text = "";
        if (!f.r.exists) {
            f.verdict = Verdict::NotThere;
        } else if (!read) {
            f.verdict = Verdict::SizeOnly;
        } else if (!f.r.readOk) {
            f.verdict = Verdict::CannotRead;
        } else {
            text = scratch;
            f.verdict = f.r.truncated ? Verdict::BadFormat : parse(k.role, i, scratch, f.r.got, out);
        }
        if (sink.file != nullptr) {
            sink.file(sink.ctx, i, f.r, f.verdict, text);
        }
    }
}

} // namespace ClockProbe
