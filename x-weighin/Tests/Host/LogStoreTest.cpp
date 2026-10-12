#include "LogStore.hpp"

#include "Calendar.hpp"
#include "support/TreeFileSystem.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace XWeighIn;

namespace
{
const std::string kOwn    = "/Apps/XWeighIn/weighins.csv";
const std::string kShared = "/Apps/SharedData/HybridX/weighins.csv";

Entry entry(int32_t day, uint16_t minute, int32_t hundredths)
{
    Entry e;
    e.day        = day;
    e.minute     = minute;
    e.hundredths = hundredths;
    return e;
}

const int32_t kOct12 = daysFromCivil(2026, 10, 12);

struct Collect : EntryVisitor {
    std::vector<Entry> entries;
    void onEntry(const Entry& e) override { entries.push_back(e); }
};
}  // namespace

TEST(LogStore, NewFileGetsAHeader)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 7 * 60 + 31, 8230)));
    EXPECT_EQ(fs.content(kOwn), "date,time,kg\n2026-10-12,07:31,82.30\n");
}

TEST(LogStore, AppendsInOrder)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 451, 8230)));
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12 + 1, 452, 8215)));
    EXPECT_EQ(fs.content(kOwn),
              "date,time,kg\n2026-10-12,07:31,82.30\n2026-10-13,07:32,82.15\n");
}

TEST(LogStore, EmptyFileGetsAHeader)
{
    TreeFileSystem fs;
    fs.addFile(kOwn, "");
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 451, 8230)));
    EXPECT_EQ(fs.content(kOwn), "date,time,kg\n2026-10-12,07:31,82.30\n");
}

TEST(LogStore, RefusesAnInvalidEntry)
{
    TreeFileSystem fs;
    EXPECT_FALSE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 451, 0)));
    EXPECT_FALSE(fs.hasFile(kOwn));
}

TEST(LogStore, StepsOverATornLastLine)
{
    TreeFileSystem fs;
    fs.addFile(kOwn, "date,time,kg\n2026-10-12,07:31,82.30\n2026-10-13,07:");
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12 + 2, 452, 8190)));
    EXPECT_EQ(fs.content(kOwn),
              "date,time,kg\n2026-10-12,07:31,82.30\n2026-10-13,07:\n2026-10-14,07:32,81.90\n");

    Collect c;
    const LogStore::ScanResult r = LogStore::scan(fs, LogStore::kLogFile, c);
    EXPECT_TRUE(r.opened);
    EXPECT_EQ(r.entries, 2u);
    EXPECT_EQ(r.rejected, 1u);
    ASSERT_EQ(c.entries.size(), 2u);
    EXPECT_EQ(c.entries[1].hundredths, 8190);
}

TEST(LogStore, ScanMissingFile)
{
    TreeFileSystem fs;
    Collect c;
    const LogStore::ScanResult r = LogStore::scan(fs, LogStore::kLogFile, c);
    EXPECT_FALSE(r.opened);
    EXPECT_EQ(r.entries, 0u);
    EXPECT_TRUE(c.entries.empty());
}

TEST(LogStore, ScanStreamsAcrossChunks)
{
    // 500 lines is about 11 KB: many times the 256-byte read chunk, so lines
    // straddle chunk boundaries at every offset.
    TreeFileSystem fs;
    for (int i = 0; i < 500; ++i) {
        ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile,
                                     entry(kOct12 + i, static_cast<uint16_t>(i % 1440), 7000 + i)));
    }
    Collect c;
    const LogStore::ScanResult r = LogStore::scan(fs, LogStore::kLogFile, c);
    EXPECT_EQ(r.entries, 500u);
    EXPECT_EQ(r.rejected, 0u);
    ASSERT_EQ(c.entries.size(), 500u);
    for (int i = 0; i < 500; ++i) {
        EXPECT_EQ(c.entries[i].day, kOct12 + i);
        EXPECT_EQ(c.entries[i].hundredths, 7000 + i);
    }
}

TEST(LogStore, ScanCountsJunkButSkipsHeaderAndBlankLines)
{
    TreeFileSystem fs;
    fs.addFile(kOwn,
               "date,time,kg\r\n"
               "2026-10-12,07:31,82.30\r\n"
               "\r\n"
               "\n"
               "not a weigh-in\n" +
               std::string(100, 'x') + "\n"
               "2026-10-13,07:31,82.10");   // a last line with no newline still counts
    Collect c;
    const LogStore::ScanResult r = LogStore::scan(fs, LogStore::kLogFile, c);
    EXPECT_EQ(r.entries, 2u);
    EXPECT_EQ(r.rejected, 2u);
    ASSERT_EQ(c.entries.size(), 2u);
    EXPECT_EQ(c.entries[1].hundredths, 8210);
}

TEST(LogStore, AppendBothWritesTheSharedCopy)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12, 451, 8230)));
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12 + 1, 451, 8220)));
    ASSERT_TRUE(fs.hasFile(kShared));
    EXPECT_EQ(fs.content(kShared), fs.content(kOwn));
}

TEST(LogStore, AppendBothSucceedsWithoutSharedData)
{
    // A file where the HybridX folder should be: the folder cannot be made,
    // as when SharedData is unavailable.
    TreeFileSystem fs;
    fs.addFile("/Apps/SharedData/HybridX", "in the way");
    EXPECT_TRUE(LogStore::appendBoth(fs, entry(kOct12, 451, 8230)));
    EXPECT_TRUE(fs.hasFile(kOwn));
    EXPECT_FALSE(fs.hasFile(kShared));
}

TEST(LogStore, ReconcileNothingToDo)
{
    TreeFileSystem fs;
    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Nothing);
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12, 451, 8230)));
    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Nothing);
}

TEST(LogStore, ReconcileRestoresAfterAReinstall)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12, 451, 8230)));
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12 + 1, 451, 8220)));
    const std::string before = fs.content(kOwn);
    ASSERT_TRUE(fs.remove("weighins.csv"));   // uninstalled and installed again

    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Restored);
    EXPECT_EQ(fs.content(kOwn), before);
    EXPECT_FALSE(fs.hasFile(kOwn + ".tmp"));
}

TEST(LogStore, ReconcileCreatesTheMissingCopy)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 451, 8230)));
    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Refreshed);
    EXPECT_EQ(fs.content(kShared), fs.content(kOwn));
    EXPECT_FALSE(fs.hasFile(kShared + ".tmp"));
}

TEST(LogStore, ReconcileRefreshesADifferentCopy)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::appendBoth(fs, entry(kOct12, 451, 8230)));
    // A weigh-in that reached our own log but not the copy.
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12 + 1, 451, 8220)));
    EXPECT_NE(fs.content(kShared), fs.content(kOwn));

    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Refreshed);
    EXPECT_EQ(fs.content(kShared), fs.content(kOwn));
}

TEST(LogStore, ReconcileFailureLeavesOurLogAlone)
{
    TreeFileSystem fs;
    ASSERT_TRUE(LogStore::append(fs, LogStore::kLogFile, entry(kOct12, 451, 8230)));
    const std::string before = fs.content(kOwn);
    fs.addFile("/Apps/SharedData/HybridX", "in the way");   // the folder cannot be made
    EXPECT_EQ(LogStore::reconcile(fs), LogStore::Reconcile::Failed);
    EXPECT_EQ(fs.content(kOwn), before);
}

TEST(LogStore, CopyFromAMissingFileChangesNothing)
{
    TreeFileSystem fs;
    fs.addFile("/Apps/XWeighIn/target.csv", "keep me");
    EXPECT_FALSE(LogStore::copyFile(fs, "missing.csv", "target.csv"));
    EXPECT_EQ(fs.content("/Apps/XWeighIn/target.csv"), "keep me");
    EXPECT_FALSE(fs.hasFile("/Apps/XWeighIn/target.csv.tmp"));
}

TEST(LogStore, CopyReplacesAnExistingFile)
{
    // FatFs rename refuses an existing destination: the copy must remove it first.
    TreeFileSystem fs;
    fs.addFile("/Apps/XWeighIn/a.csv", "new content");
    fs.addFile("/Apps/XWeighIn/b.csv", "old");
    EXPECT_TRUE(LogStore::copyFile(fs, "a.csv", "b.csv"));
    EXPECT_EQ(fs.content("/Apps/XWeighIn/b.csv"), "new content");
}
