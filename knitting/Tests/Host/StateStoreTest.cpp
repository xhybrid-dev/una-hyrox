#include <gtest/gtest.h>

#include "StateStore.hpp"
#include "TreeFileSystem.hpp"

using Knitting::CounterState;
namespace Store = Knitting::StateStore;

namespace
{
CounterState make(uint32_t rows, uint32_t set, uint32_t total)
{
    CounterState s;
    s.rows = rows;
    s.set = set;
    s.total = total;
    return s;
}

const char* kGood = "knitting 1\nrows=3\nset=2\ntotal=11\n";
} // namespace

TEST(StateStore, NothingSavedGivesAFreshCounter)
{
    TreeFileSystem fs;
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::Fresh);
    EXPECT_EQ(l.state.rows, 0u);
    EXPECT_EQ(l.state.set, 1u);
    EXPECT_EQ(l.state.total, 0u);
}

TEST(StateStore, ASavedStateComesBack)
{
    TreeFileSystem fs;
    ASSERT_TRUE(Store::save(fs, make(3, 2, 11)));
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::File);
    EXPECT_EQ(l.state.rows, 3u);
    EXPECT_EQ(l.state.set, 2u);
    EXPECT_EQ(l.state.total, 11u);
}

TEST(StateStore, TheLatestSaveWinsAndTheFileIsReadableText)
{
    TreeFileSystem fs;
    ASSERT_TRUE(Store::save(fs, make(1, 1, 1)));
    ASSERT_TRUE(Store::save(fs, make(2, 1, 2)));
    ASSERT_TRUE(Store::save(fs, make(3, 1, 3)));
    EXPECT_EQ(fs.content("/Apps/Knitting/state.txt"), "knitting 1\nrows=3\nset=1\ntotal=3\n");
    EXPECT_EQ(Store::load(fs).state.rows, 3u);
}

TEST(StateStore, ACorruptFileFallsBackToTheBackup)
{
    TreeFileSystem fs;
    ASSERT_TRUE(Store::save(fs, make(2, 1, 2)));
    ASSERT_TRUE(Store::save(fs, make(3, 1, 3)));      // the first save is now state.txt.bak
    fs.addFile("/Apps/Knitting/state.txt", "garbage");
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::Backup);
    EXPECT_EQ(l.state.rows, 2u);
}

TEST(StateStore, AMissingFileFallsBackToTheBackup)
{
    TreeFileSystem fs;
    fs.addFile("/Apps/Knitting/state.txt.bak", kGood);
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::Backup);
    EXPECT_EQ(l.state.rows, 3u);
    EXPECT_EQ(l.state.set, 2u);
    EXPECT_EQ(l.state.total, 11u);
}

TEST(StateStore, BothUnusableGivesAFreshCounter)
{
    TreeFileSystem fs;
    fs.addFile("/Apps/Knitting/state.txt", "junk");
    fs.addFile("/Apps/Knitting/state.txt.bak", "more junk");
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::Fresh);
    EXPECT_EQ(l.state.set, 1u);
}

TEST(StateStore, AHandEditedOutOfRangeFileIsClamped)
{
    TreeFileSystem fs;
    fs.addFile("/Apps/Knitting/state.txt", "knitting 1\nrows=123456\nset=7\ntotal=5\n");
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.source, Store::Source::File);
    EXPECT_EQ(l.state.rows, Knitting::kMaxRows);
    EXPECT_EQ(l.state.set, 7u);
}

TEST(StateStore, AFailedSaveKeepsThePreviousCopy)
{
    TreeFileSystem fs;
    ASSERT_TRUE(Store::save(fs, make(3, 2, 11)));
    // A directory where the temp file must go makes staging fail.
    fs.addDir("/Apps/Knitting/state.txt.tmp");
    EXPECT_FALSE(Store::save(fs, make(4, 2, 12)));
    const Store::Loaded l = Store::load(fs);
    EXPECT_EQ(l.state.rows, 3u);
    EXPECT_EQ(l.state.total, 11u);
}
