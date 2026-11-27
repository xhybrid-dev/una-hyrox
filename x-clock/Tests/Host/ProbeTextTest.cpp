#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "PretendWatch.hpp"
#include "ProbeText.hpp"

using namespace ClockProbe;

namespace
{
ScanData scanOf(TreeFileSystem& fs)
{
    static char scratch[kScratchBytes];
    ScanData d;
    runScan(fs, scratch, sizeof(scratch), d, ScanSink {});
    return d;
}

void expectFits(const ViewRow& r)
{
    EXPECT_LT(std::strlen(r.text), static_cast<size_t>(kRowChars)) << r.text;
    EXPECT_GT(std::strlen(r.text), 0u);
}
} // namespace

// ---- the log files ----------------------------------------------------------------------------

TEST(LogText, AMinuteRowHasOneFieldPerHeaderColumn)
{
    MinuteRow r;
    r.utc = 1795000060;
    r.hour = 9;
    r.minute = 5;
    r.wristOn = true;
    r.battX100 = 6653;
    r.mv = 3950;
    r.ma = -1;
    r.avgMa = -2;
    r.mah = 88;
    r.charging = 0;
    r.steps = 1812;
    r.active = 11;
    r.floors = 4;
    r.wristToday = 3;
    r.gestureToday = 1;
    r.reports = 12;
    r.fpsX10 = 100;
    r.maxGapMs = 5000;
    r.resumes = 2;
    r.suspends = 1;
    r.starts = 4;
    char buf[256];
    const size_t n = formatMinuteRow(buf, sizeof(buf), r);
    EXPECT_EQ(std::string(buf, n), "1795000060,09:05,on,6653,3950,-1,-2,88,0,1812,11,4,3,1,12,100,5000,2,1,4\n");
    auto commas = [](const std::string& s) { return std::count(s.begin(), s.end(), ','); };
    EXPECT_EQ(commas(std::string(buf, n)), commas(minuteHeader()));
}

TEST(LogText, UnknownFiguresAreLeftEmptyNotZero)
{
    MinuteRow r;
    r.utc = 5;
    char buf[256];
    const size_t n = formatMinuteRow(buf, sizeof(buf), r);
    const std::string row(buf, n);
    EXPECT_NE(row.find(",off,,,,,,,"), std::string::npos) << row;   // battery to charging all empty
    auto commas = [](const std::string& s) { return std::count(s.begin(), s.end(), ','); };
    EXPECT_EQ(commas(row), commas(minuteHeader()));
}

TEST(LogText, ADaysRowAndItsHeader)
{
    char buf[64];
    const size_t n = formatDaysRow(buf, sizeof(buf), 20261126, 18, 8123, 41, 12, false);
    EXPECT_EQ(std::string(buf, n), "20261126,18,8123,41,12,off\n");
    EXPECT_STREQ(daysHeader(), "date,hour,steps,active_min,floors,wrist_mode\n");
}

TEST(LogText, AnEventLineKeepsTheCsvIntact)
{
    char buf[128];
    const size_t n = formatEventLine(buf, sizeof(buf), 1795000000, 42, "wrist", "a,b\nc");
    EXPECT_EQ(std::string(buf, n), "1795000000,42,wrist,a;b c\n");
}

TEST(LogText, NoWriterEverPassesItsBuffer)
{
    char tiny[8];
    MinuteRow r;
    r.utc = 1795000060;
    const size_t a = formatMinuteRow(tiny, sizeof(tiny), r);
    EXPECT_LT(a, sizeof(tiny));
    EXPECT_EQ(tiny[a], '\0');
    const size_t b = formatEventLine(tiny, sizeof(tiny), 1795000000, 99999, "start", "a long detail");
    EXPECT_LT(b, sizeof(tiny));
    const size_t c = formatFileContent(tiny, sizeof(tiny), "abcdefghijklmnopqrstuvwxyz", 100);
    EXPECT_LT(c, sizeof(tiny));
    const size_t d = formatScanHeader(tiny, sizeof(tiny), "2026-11-26T18:42", "start");
    EXPECT_LT(d, sizeof(tiny));
}

TEST(LogText, TheScanDumpIsReadableAndShowsWhatMatters)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const ScanData d = scanOf(fs);
    char buf[256];
    size_t n = formatScanHeader(buf, sizeof(buf), "2026-11-26T18:42", "start");
    EXPECT_EQ(std::string(buf, n), "== scan 2026-11-26T18:42 (start) ==\n");
    n = formatListHeader(buf, sizeof(buf), kHybridxDir, d.hybridx);
    EXPECT_NE(std::string(buf, n).find("opened=1"), std::string::npos);
    EXPECT_NE(std::string(buf, n).find("entries=12"), std::string::npos);
    n = formatFileLine(buf, sizeof(buf), kKnown[0], d.files[0].r, d.files[0].verdict);
    EXPECT_NE(std::string(buf, n).find("file act_run: exists, size "), std::string::npos);
    EXPECT_NE(std::string(buf, n).find("read ok, parsed"), std::string::npos);
    n = formatFileLine(buf, sizeof(buf), kKnown[10], d.files[10].r, d.files[10].verdict);
    EXPECT_NE(std::string(buf, n).find("size only"), std::string::npos);
    EXPECT_EQ(std::string(buf, n).find("read"), std::string::npos) << "a size-only file is never read";
}

TEST(LogText, AMissingFileAndAFlaggedOpenWithoutExistAreSaidPlainly)
{
    TreeFileSystem fs(pretend::kSandbox);
    const ScanData d = scanOf(fs);
    char buf[160];
    size_t n = formatFileLine(buf, sizeof(buf), kKnown[0], d.files[0].r, d.files[0].verdict);
    EXPECT_STREQ(buf, "file act_run: not there\n");
    EXPECT_EQ(n, std::strlen(buf));
    ListResult odd;
    odd.openedWithoutExist = true;
    n = formatListHeader(buf, sizeof(buf), kHybridxDir, odd);
    EXPECT_NE(std::string(buf, n).find("OPEN-WITHOUT-EXIST"), std::string::npos);
}

TEST(LogText, FileContentIsIndentedAndCutAtTheLimit)
{
    char buf[128];
    const size_t n = formatFileContent(buf, sizeof(buf), "{\"v\":1}\n{\"x\":2}", 100);
    EXPECT_EQ(std::string(buf, n), "    {\"v\":1} {\"x\":2}\n");
    const size_t m = formatFileContent(buf, sizeof(buf), "0123456789", 4);
    EXPECT_EQ(std::string(buf, m), "    0123\n");
}

// ---- the screen -------------------------------------------------------------------------------

TEST(ScreenText, FilesRowCountsAndColours)
{
    ViewRow r;
    TreeFileSystem full(pretend::kSandbox);
    pretend::addFamily(full);
    rowFiles(r, scanOf(full));
    EXPECT_STREQ(r.text, "Files 12 of 12");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));

    TreeFileSystem empty(pretend::kSandbox);
    rowFiles(r, scanOf(empty));
    EXPECT_STREQ(r.text, "Files 0 of 12");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Red));

    TreeFileSystem bad(pretend::kSandbox);
    pretend::addFamily(bad);
    bad.addFile("/Apps/SharedData/HybridX/vo2max.json", "nonsense", 1);
    rowFiles(r, scanOf(bad));
    EXPECT_STREQ(r.text, "Files 12 of 12, 1 bad");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));
    expectFits(r);
}

TEST(ScreenText, ListRowCoversEveryOutcome)
{
    ViewRow r;
    ScanData d;
    d.shared.exists = d.shared.opened = true;
    d.shared.entries = 3;
    d.hybridx.exists = d.hybridx.opened = true;
    d.hybridx.entries = 12;
    rowList(r, d);
    EXPECT_STREQ(r.text, "List ok 3 + 12");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));

    d.hybridx = ListResult {};
    rowList(r, d);
    EXPECT_STREQ(r.text, "List ok 3, no HX");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));

    d.shared = ListResult {};
    d.shared.exists = true;   // there, but would not open
    rowList(r, d);
    EXPECT_STREQ(r.text, "List denied");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Red));

    d.shared = ListResult {};
    d.shared.openedWithoutExist = true;
    rowList(r, d);
    EXPECT_STREQ(r.text, "List: open w/o dir");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));

    d.shared = ListResult {};
    rowList(r, d);
    EXPECT_STREQ(r.text, "List: no folder");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Red));
}

TEST(ScreenText, LogRow)
{
    ViewRow r;
    rowLog(r, -1);
    EXPECT_STREQ(r.text, "Log write ?");
    rowLog(r, 1);
    EXPECT_STREQ(r.text, "Log write ok");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));
    rowLog(r, 0);
    EXPECT_STREQ(r.text, "Log write FAILED");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Red));
}

TEST(ScreenText, AgeIsShort)
{
    char b[8];
    formatAge(b, sizeof(b), 4000);
    EXPECT_STREQ(b, "4s");
    formatAge(b, sizeof(b), 59999);
    EXPECT_STREQ(b, "59s");
    formatAge(b, sizeof(b), 60000);
    EXPECT_STREQ(b, "1m");
    formatAge(b, sizeof(b), 3 * 3600 * 1000u);
    EXPECT_STREQ(b, "3h");
    formatAge(b, sizeof(b), 3 * 86400 * 1000u);
    EXPECT_STREQ(b, "3d");
}

TEST(ScreenText, WristRow)
{
    ViewRow r;
    rowWrist(r, false, 0, false, 0);
    EXPECT_STREQ(r.text, "Wrist off today");
    rowWrist(r, true, 0, false, 0xFFFFFFFFu);
    EXPECT_STREQ(r.text, "Wrist 0 so far");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Grey));
    rowWrist(r, true, 12, true, 4000);
    EXPECT_STREQ(r.text, "Wrist 12, 4s ago");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));
    rowWrist(r, true, 123456, true, 5 * 86400 * 1000u);
    EXPECT_STREQ(r.text, "Wrist 999, 5d ago");
    expectFits(r);
}

TEST(ScreenText, TickRow)
{
    ViewRow r;
    rowFrames(r, false, 0, 0);
    EXPECT_STREQ(r.text, "Ticks: waiting");
    rowFrames(r, true, 100, 3);
    EXPECT_STREQ(r.text, "Ticks 10.0/s, R3");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));
    rowFrames(r, true, 57, 120);
    EXPECT_STREQ(r.text, "Ticks 5.7/s, R99");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));
    rowFrames(r, true, 0xFFFF, 0xFFFF);
    expectFits(r);
}

TEST(ScreenText, PlanRowShowsTodaysSessionFromTheRealFile)
{
    ViewRow r;
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const ScanData d = scanOf(fs);
    rowPlan(r, d, 20261126);
    EXPECT_STREQ(r.text, "4 x 4 min 4:18/km");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));
    rowPlan(r, d, 20261125);
    EXPECT_STREQ(r.text, "Rest day");
    rowPlan(r, d, 20300101);
    EXPECT_STREQ(r.text, "Plan: open X Training");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));
    rowPlan(r, d, 0);
    EXPECT_STREQ(r.text, "Plan: no clock");
    expectFits(r);
}

TEST(ScreenText, PlanRowWhenDoneNoFileOrABadFile)
{
    ViewRow r;
    {
        TreeFileSystem fs(pretend::kSandbox);
        fs.addFile("/Apps/SharedData/HybridX/training_today.json",
                   pretend::todayText(20261126, XTraining::TodayStatus::Done), 1);
        rowPlan(r, scanOf(fs), 20261126);
        EXPECT_STREQ(r.text, "Done: 4 x 4 min");
    }
    {
        TreeFileSystem fs(pretend::kSandbox);
        rowPlan(r, scanOf(fs), 20261126);
        EXPECT_STREQ(r.text, "Plan: no file");
        EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Grey));
    }
    {
        TreeFileSystem fs(pretend::kSandbox);
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", "{oops", 1);
        rowPlan(r, scanOf(fs), 20261126);
        EXPECT_STREQ(r.text, "Plan: bad file");
        EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Red));
    }
}

TEST(ScreenText, AnOverlongSessionKeepsItsWordsAndDropsThePace)
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
    ViewRow r;
    rowPlan(r, scanOf(fs), 20261126);
    EXPECT_STREQ(r.text, "100 min, 20 fast");   // 16 + 1 + 8 = 25 characters would reach the ring
    expectFits(r);
}

TEST(ScreenText, ATwentyCharacterPlanKeepsItsPaceAndTwentyOneDropsIt)
{
    auto planOf = [](const char* what, const char* pace) {
        TreeFileSystem fs(pretend::kSandbox);
        XTraining::TodayData t;
        t.plan = 1; t.weeks = 10; t.start = 20261012; t.end = 20261220; t.first = 20261123; t.last = 20261206;
        t.count = 1;
        t.day[0].date = 20261126;
        t.day[0].kind = XTraining::Kind::Easy;
        t.day[0].runWeek = 3;
        XTraining::TodayFile::setText(t.day[0].what, sizeof(t.day[0].what), what);
        XTraining::TodayFile::setText(t.day[0].pace, sizeof(t.day[0].pace), pace);
        char buf[XTraining::TodayFile::kMaxJsonBytes + 64];
        const size_t n = XTraining::TodayFile::write(t, buf, sizeof(buf));
        fs.addFile("/Apps/SharedData/HybridX/training_today.json", std::string(buf, n), 1);
        ViewRow r;
        rowPlan(r, scanOf(fs), 20261126);
        return std::string(r.text);
    };
    EXPECT_EQ(planOf("12 x 400 m", "1:23/400m"), "12 x 400 m 1:23/400m");    // 10 + 1 + 9 = 20
    EXPECT_EQ(planOf("12 x 400 m w", "1:23/400m"), "12 x 400 m w");           // 21: the pace goes
}

TEST(ScreenText, Vo2RowShowsTheFigureAndItsChange)
{
    ViewRow r;
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    rowVo2(r, scanOf(fs));
    EXPECT_STREQ(r.text, "VO2 52.3 (+0.4)");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Green));

    fs.addFile("/Apps/SharedData/HybridX/vo2max.json",
               "{\"v\":2,\"vo2maxX10\":501,\"runs\":6,\"utc\":1791000000,\"beforeX10\":523,\"trend\":[523,501]}", 1);
    rowVo2(r, scanOf(fs));
    EXPECT_STREQ(r.text, "VO2 50.1 (-2.2)");

    fs.addFile("/Apps/SharedData/HybridX/vo2max.json",
               "{\"v\":2,\"vo2maxX10\":501,\"runs\":1,\"utc\":1791000000,\"beforeX10\":0,\"trend\":[501]}", 1);
    rowVo2(r, scanOf(fs));
    EXPECT_STREQ(r.text, "VO2 50.1");
    expectFits(r);
}

TEST(ScreenText, Vo2RowWhenThereIsNothing)
{
    ViewRow r;
    TreeFileSystem fs(pretend::kSandbox);
    rowVo2(r, scanOf(fs));
    EXPECT_STREQ(r.text, "VO2: no file");
    fs.addFile("/Apps/SharedData/HybridX/vo2max.json", "{\"v\":2,\"runs\":0}", 1);
    rowVo2(r, scanOf(fs));
    EXPECT_STREQ(r.text, "VO2: no data");
    EXPECT_EQ(r.light, static_cast<uint8_t>(Light::Amber));
}

TEST(ScreenText, EveryRowFitsTheFaceInItsWorstCase)
{
    // The longest each row can be, built from the largest inputs it takes.
    ViewRow r;
    ScanData d;
    for (ScanFile& f : d.files) {
        f.r.exists = true;
        f.verdict = Verdict::BadFormat;
    }
    rowFiles(r, d);
    expectFits(r);
    d.shared.exists = d.shared.opened = d.hybridx.exists = d.hybridx.opened = true;
    d.shared.entries = d.hybridx.entries = 65535;
    rowList(r, d);
    expectFits(r);
    rowWrist(r, true, 0xFFFFFFFFu, true, 0);
    expectFits(r);
    rowFrames(r, true, 0xFFFF, 0xFFFF);
    expectFits(r);
    // Even the widest of them is clear of the bottom row's chord (about 150 px at 7.5 px a character).
    EXPECT_LE(kRowChars - 1, 23);
}

TEST(ScreenText, TheRingTakesTheWeeksGrades)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    const WeekView w = makeWeek(scanOf(fs), pretend::thursday());
    ViewData v;
    ringFromWeek(v, w);
    EXPECT_EQ(v.today, 3);
    EXPECT_EQ(v.level[0], static_cast<uint8_t>(Level::High));
    EXPECT_EQ(v.level[1], static_cast<uint8_t>(Level::Low));
    EXPECT_EQ(v.level[4], static_cast<uint8_t>(Level::None));
}

TEST(ScreenText, TheTrendIsScaledToTheBarsHeight)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    ViewData v;
    trendFromVo2(v, scanOf(fs));
    ASSERT_EQ(v.trendCount, 4);
    EXPECT_EQ(v.trend[0], 2);    // 50.5 is the lowest
    EXPECT_EQ(v.trend[3], 20);   // 52.3 the highest
    for (uint8_t i = 0; i < v.trendCount; ++i) {
        EXPECT_GE(v.trend[i], 2);
        EXPECT_LE(v.trend[i], 20);
    }
}

TEST(ScreenText, AFlatTrendIsMidHeightAndNoFileIsNoBars)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/vo2max.json",
               "{\"v\":2,\"vo2maxX10\":500,\"runs\":2,\"utc\":1791000000,\"beforeX10\":500,\"trend\":[500,500]}", 1);
    ViewData v;
    trendFromVo2(v, scanOf(fs));
    ASSERT_EQ(v.trendCount, 2);
    EXPECT_EQ(v.trend[0], 11);
    TreeFileSystem none(pretend::kSandbox);
    trendFromVo2(v, scanOf(none));
    EXPECT_EQ(v.trendCount, 0);
}
