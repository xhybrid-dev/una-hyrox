#include <gtest/gtest.h>

#include <map>
#include <string>

#include "PretendWatch.hpp"
#include "Scan.hpp"

using namespace ClockProbe;

namespace
{
struct Seen {
    std::map<std::string, std::string> listed;   // dir -> names joined
    std::map<uint8_t, std::string>     text;     // file index -> content handed to the sink
    int                                files = 0;
};

void onListed(void* ctx, const char* dir, const ListEntry& e)
{
    static_cast<Seen*>(ctx)->listed[dir] += std::string(e.name) + (e.isDir ? "/ " : " ");
}

void onFile(void* ctx, uint8_t index, const FileResult&, Verdict, const char* text)
{
    Seen* s = static_cast<Seen*>(ctx);
    ++s->files;
    s->text[index] = text;
}

ScanData scan(TreeFileSystem& fs, Seen& seen)
{
    static char scratch[kScratchBytes];
    ScanData d;
    ScanSink sink;
    sink.listed = &onListed;
    sink.file   = &onFile;
    sink.ctx    = &seen;
    runScan(fs, scratch, sizeof(scratch), d, sink);
    return d;
}
} // namespace

TEST(Scan, AWatchWithTheWholeFamilyParsesEveryFileWithTheFamilysOwnReaders)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    Seen seen;
    const ScanData d = scan(fs, seen);

    EXPECT_EQ(d.found(), kKnownCount);
    EXPECT_EQ(d.problems(), 0u);
    for (uint8_t i = 0; i < kLinkFiles; ++i) {
        EXPECT_EQ(d.files[i].verdict, Verdict::Parsed) << kKnown[i].label;
        EXPECT_TRUE(d.actOk[i]);
        EXPECT_GE(d.act[i].count, 1);
    }
    EXPECT_TRUE(d.streakNowOk);
    EXPECT_EQ(d.streakNow.target, 4);
    EXPECT_EQ(d.streakNow.streak, 8);
    EXPECT_TRUE(d.todayOk);
    EXPECT_EQ(d.today.plan, 1);
    EXPECT_TRUE(d.vo2Ok);
    EXPECT_EQ(d.vo2.vo2x10, 523);
    EXPECT_EQ(d.vo2.beforeX10, 519);
    EXPECT_EQ(d.vo2.count, 4);
}

TEST(Scan, ListsBothFoldersAndSeesEverythingThatIsThere)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    Seen seen;
    const ScanData d = scan(fs, seen);
    EXPECT_TRUE(d.shared.opened);
    EXPECT_TRUE(d.hybridx.opened);
    // A file that is not ours shows in the listing: that is what the listing is for.
    EXPECT_NE(seen.listed["../SharedData/HybridX"].find("someone_else.dat"), std::string::npos);
    EXPECT_NE(seen.listed["../SharedData"].find("HybridX/"), std::string::npos);
    EXPECT_NE(seen.listed["../SharedData"].find("stride.json"), std::string::npos);
}

TEST(Scan, NothingPrivateIsHandedToTheLog)
{
    TreeFileSystem fs(pretend::kSandbox);
    pretend::addFamily(fs);
    Seen seen;
    const ScanData d = scan(fs, seen);
    for (uint8_t i = 0; i < kKnownCount; ++i) {
        if (kKnown[i].role == Role::SizeOnly) {
            EXPECT_EQ(d.files[i].verdict, Verdict::SizeOnly) << kKnown[i].label;
            EXPECT_TRUE(d.files[i].r.exists);
            EXPECT_FALSE(d.files[i].r.opened) << kKnown[i].label << " must not be opened";
            EXPECT_EQ(seen.text[i], "") << kKnown[i].label << " content must not reach the log";
        } else {
            EXPECT_FALSE(seen.text[i].empty()) << kKnown[i].label;
        }
    }
}

TEST(Scan, AnEmptyWatchFindsNothingAndDoesNotCrash)
{
    TreeFileSystem fs(pretend::kSandbox);
    Seen seen;
    const ScanData d = scan(fs, seen);
    EXPECT_EQ(d.found(), 0u);
    EXPECT_EQ(d.problems(), 0u);
    EXPECT_FALSE(d.shared.opened);
    EXPECT_FALSE(d.todayOk);
    EXPECT_FALSE(d.vo2Ok);
    EXPECT_EQ(seen.files, kKnownCount);   // each file is still reported, as not there
}

TEST(Scan, AFileTheReaderRefusesIsBadFormatNotMissing)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/act_run.json", "{\"v\":9,\"app\":\"X\",\"a\":[]}", 1);
    fs.addFile("/Apps/SharedData/HybridX/vo2max.json", "{\"v\":1,\"vo2maxX10\":500}", 1);
    fs.addFile("/Apps/SharedData/HybridX/training_today.json", "not json at all", 1);
    Seen seen;
    const ScanData d = scan(fs, seen);
    EXPECT_EQ(d.files[0].verdict, Verdict::BadFormat);
    EXPECT_FALSE(d.actOk[0]);
    EXPECT_EQ(d.files[kIdxVo2].verdict, Verdict::BadFormat);   // a version 1 file has no trend
    EXPECT_EQ(d.files[kIdxToday].verdict, Verdict::BadFormat);
    EXPECT_EQ(d.problems(), 3u);
}

TEST(Scan, AFileBiggerThanTheBufferIsBadFormatAndSaysTruncated)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/act_run.json", std::string(kScratchBytes + 100, ' '), 1);
    Seen seen;
    const ScanData d = scan(fs, seen);
    EXPECT_TRUE(d.files[0].r.truncated);
    EXPECT_EQ(d.files[0].verdict, Verdict::BadFormat);
}

TEST(Scan, TheTodayFileCarriesAPlanForToday)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/training_today.json", pretend::todayText(20261126), 1);
    Seen seen;
    const ScanData d = scan(fs, seen);
    ASSERT_TRUE(d.todayOk);
    const auto pick = XTraining::TodayFile::pick(&d.today, 20261126);
    EXPECT_EQ(pick.state, XTraining::TodayFile::State::Session);
    ASSERT_NE(pick.today, nullptr);
    EXPECT_STREQ(pick.today->what, "4 x 4 min");
    EXPECT_STREQ(pick.today->pace, "4:18/km");
}

TEST(Scan, ReusingOneScanDataLeavesNothingOfThePreviousScan)
{
    // The service scans into the same object every time; a second scan of a changed watch must not
    // keep the first one's parsed files.
    static char scratch[kScratchBytes];
    static ScanData d;
    TreeFileSystem full(pretend::kSandbox);
    pretend::addFamily(full);
    runScan(full, scratch, sizeof(scratch), d, ScanSink {});
    ASSERT_EQ(d.found(), kKnownCount);
    ASSERT_TRUE(d.todayOk);
    ASSERT_TRUE(d.vo2Ok);

    TreeFileSystem empty(pretend::kSandbox);
    runScan(empty, scratch, sizeof(scratch), d, ScanSink {});
    EXPECT_EQ(d.found(), 0u);
    EXPECT_FALSE(d.todayOk);
    EXPECT_FALSE(d.vo2Ok);
    EXPECT_FALSE(d.streakNowOk);
    EXPECT_EQ(d.today.plan, 0);
    EXPECT_EQ(d.vo2.count, 0);
    for (uint8_t i = 0; i < kLinkFiles; ++i) {
        EXPECT_FALSE(d.actOk[i]);
        EXPECT_EQ(d.act[i].count, 0);
    }
    EXPECT_FALSE(d.shared.opened);
}
