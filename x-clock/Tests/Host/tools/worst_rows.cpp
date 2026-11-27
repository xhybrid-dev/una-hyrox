/**
 * worst_rows: prints the longest text each of the probe's seven screen rows can show, one per line as
 * "<row> <text>", for docs/experiments/audit.py to measure against the round face. The cases are the
 * widest each row's own limits allow (the listing stops at 64 entries, the plan's words at 20
 * characters and its pace at 12, and so on).
 */

#include <cstdio>
#include <cstring>

#include "PretendWatch.hpp"
#include "ProbeText.hpp"

using namespace ClockProbe;

namespace
{
void out(int row, const ViewRow& r)
{
    std::printf("%d %s\n", row, r.text);
}

ScanData scanOf(TreeFileSystem& fs)
{
    static char scratch[kScratchBytes];
    ScanData d;
    runScan(fs, scratch, sizeof(scratch), d, ScanSink {});
    return d;
}
} // namespace

int main()
{
    ViewRow r;

    // Row 0: files, with every one bad.
    {
        ScanData d;
        for (ScanFile& f : d.files) {
            f.r.exists = true;
            f.verdict  = Verdict::BadFormat;
        }
        rowFiles(r, d);
        out(0, r);
    }
    // Row 1: the listing, at its limit of 64 entries each; and each of the other outcomes.
    {
        ScanData d;
        d.shared.exists = d.shared.opened = d.hybridx.exists = d.hybridx.opened = true;
        d.shared.entries = d.hybridx.entries = 64;
        rowList(r, d);
        out(1, r);
        d.hybridx = ListResult {};
        rowList(r, d);
        out(1, r);
        d.shared = ListResult {};
        d.shared.openedWithoutExist = true;
        rowList(r, d);
        out(1, r);
        d.shared = ListResult {};
        rowList(r, d);
        out(1, r);
        d.shared.exists = true;
        rowList(r, d);
        out(1, r);
    }
    // Row 2: the log.
    rowLog(r, 0);
    out(2, r);
    // Row 3: the wrist, at its widest count and age.
    rowWrist(r, true, 999, true, 59u * 1000u);
    out(3, r);
    rowWrist(r, true, 999, true, 23u * 3600u * 1000u);
    out(3, r);
    rowWrist(r, true, 999, true, 99u * 86400u * 1000u);
    out(3, r);
    // Row 4: the ticks.
    rowFrames(r, true, 999, 99);
    out(4, r);
    // Row 5: the plan, in each state, and a session with the longest words and pace that both fit.
    {
        TreeFileSystem fs(pretend::kSandbox);
        pretend::addFamily(fs);
        const ScanData d = scanOf(fs);
        for (uint32_t ymd : {20261126u, 20261125u, 20300101u, 0u}) {
            rowPlan(r, d, ymd);
            out(5, r);
        }
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", pretend::todayText(20261126, XTraining::TodayStatus::Done), 1);
        rowPlan(r, scanOf(fs), 20261126);
        out(5, r);
    }
    {
        TreeFileSystem fs(pretend::kSandbox);
        XTraining::TodayData t;
        t.plan = 1; t.weeks = 10; t.start = 20261012; t.end = 20261220; t.first = 20261123; t.last = 20261206;
        t.count = 1;
        t.day[0].date = 20261126;
        t.day[0].kind = XTraining::Kind::Easy;
        t.day[0].runWeek = 3;
        XTraining::TodayFile::setText(t.day[0].what, sizeof(t.day[0].what), "100 min, 20 fast");
        XTraining::TodayFile::setText(t.day[0].pace, sizeof(t.day[0].pace), "12:34/mi");
        char buf[XTraining::TodayFile::kMaxJsonBytes + 64];
        const size_t n = XTraining::TodayFile::write(t, buf, sizeof(buf));
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", std::string(buf, n), 1);
        rowPlan(r, scanOf(fs), 20261126);
        out(5, r);
        // 10 + 1 + 9 = 20, the longest shown with its pace.
        XTraining::TodayFile::setText(t.day[0].what, sizeof(t.day[0].what), "12 x 400 m");
        XTraining::TodayFile::setText(t.day[0].pace, sizeof(t.day[0].pace), "1:23/400m");
        const size_t m = XTraining::TodayFile::write(t, buf, sizeof(buf));
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", std::string(buf, m), 1);
        rowPlan(r, scanOf(fs), 20261126);
        out(5, r);
        // 21: over the limit, so the pace is left off.
        XTraining::TodayFile::setText(t.day[0].what, sizeof(t.day[0].what), "12 x 400 m w");
        const size_t k = XTraining::TodayFile::write(t, buf, sizeof(buf));
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", std::string(buf, k), 1);
        rowPlan(r, scanOf(fs), 20261126);
        out(5, r);
    }
    // Row 6: VO2max, the widest figure and change.
    {
        TreeFileSystem fs(pretend::kSandbox);
        fs.addFile("/Apps/SharedData/HybridX/vo2max.json",
                   "{\"v\":2,\"vo2maxX10\":888,\"runs\":9,\"utc\":1791000000,\"beforeX10\":778,"
                   "\"trend\":[700,710,720,730,740,750,760,770,778,888]}",
                   1);
        rowVo2(r, scanOf(fs));
        out(6, r);
    }
    return 0;
}
