#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "FileProbe.hpp"
#include "PretendWatch.hpp"

using namespace ClockProbe;

namespace
{
/// A file system that denies every exist() but still opens a folder: Gate 0's suspicion about the watch.
class DenyExistFs : public TreeFileSystem
{
public:
    using TreeFileSystem::TreeFileSystem;
    bool exist(const char*) const override { return false; }
};

void collect(void* ctx, const ListEntry& e)
{
    static_cast<std::vector<ListEntry>*>(ctx)->push_back(e);
}
} // namespace

TEST(ProbeFile, ReadsAFileAndKeepsItsSizeAndStamp)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/vo2max.json", "{\"v\":2}", 1795000000);
    char buf[64];
    const FileResult r = probeFile(fs, "../SharedData/HybridX/vo2max.json", buf, sizeof(buf), true);
    EXPECT_TRUE(r.exists);
    EXPECT_TRUE(r.infoOk);
    EXPECT_TRUE(r.opened);
    EXPECT_TRUE(r.readOk);
    EXPECT_FALSE(r.truncated);
    EXPECT_EQ(r.size, 7u);
    EXPECT_EQ(r.utc, 1795000000u);
    EXPECT_EQ(r.got, 7u);
    EXPECT_STREQ(buf, "{\"v\":2}");
}

TEST(ProbeFile, AMissingFileIsNotThereAndNothingIsOpened)
{
    TreeFileSystem fs(pretend::kSandbox);
    char buf[16] = "junk";
    const FileResult r = probeFile(fs, "../SharedData/HybridX/nope.json", buf, sizeof(buf), true);
    EXPECT_FALSE(r.exists);
    EXPECT_FALSE(r.opened);
    EXPECT_FALSE(r.readOk);
    EXPECT_STREQ(buf, "");
}

TEST(ProbeFile, ASizeOnlyAskNeverOpensTheFile)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/weighins.csv", "secret,weights\n", 1795000000);
    char buf[64] = "untouched";
    const FileResult r = probeFile(fs, "../SharedData/HybridX/weighins.csv", buf, sizeof(buf), false);
    EXPECT_TRUE(r.exists);
    EXPECT_EQ(r.size, 15u);
    EXPECT_FALSE(r.opened);
    EXPECT_FALSE(r.readOk);
    EXPECT_STREQ(buf, "");   // nothing was read into it
}

TEST(ProbeFile, ABiggerFileThanTheBufferIsFlaggedTruncated)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/x.json", std::string(100, 'a'), 1);
    char buf[16];
    const FileResult r = probeFile(fs, "../SharedData/x.json", buf, sizeof(buf), true);
    EXPECT_TRUE(r.readOk);
    EXPECT_TRUE(r.truncated);
    EXPECT_EQ(r.got, 15u);
    EXPECT_EQ(r.size, 100u);
}

TEST(ProbeFile, AFileOutsideTheSharedFolderIsNotReachable)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.blockParentAccess(true);
    fs.addFile("/Apps/Running/Activity/summary.json", "{}", 1);
    char buf[16];
    EXPECT_FALSE(probeFile(fs, "../Running/Activity/summary.json", buf, sizeof(buf), true).exists);
}

TEST(ListDir, NamesEachEntryAndCountsFilesAndFolders)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/stride.json", "{}", 5);
    fs.addDir("/Apps/SharedData/HybridX");
    fs.addFile("/Apps/SharedData/HybridX/act_run.json", "{}", 6);
    std::vector<ListEntry> seen;
    const ListResult r = listDir(fs, "../SharedData", &collect, &seen);
    EXPECT_TRUE(r.exists);
    EXPECT_TRUE(r.opened);
    EXPECT_EQ(r.entries, 2u);
    EXPECT_EQ(r.files, 1u);
    EXPECT_EQ(r.dirs, 1u);
    ASSERT_EQ(seen.size(), 2u);
    bool sawFile = false, sawDir = false;
    for (const auto& e : seen) {
        if (std::string(e.name) == "stride.json") {
            sawFile = !e.isDir && e.size == 2 && e.utc == 5;
        }
        if (std::string(e.name) == "HybridX") {
            sawDir = e.isDir;
        }
    }
    EXPECT_TRUE(sawFile);
    EXPECT_TRUE(sawDir);
}

TEST(ListDir, AMissingFolderIsReportedAsMissing)
{
    TreeFileSystem fs(pretend::kSandbox);
    const ListResult r = listDir(fs, "../SharedData/HybridX", nullptr, nullptr);
    EXPECT_FALSE(r.exists);
    EXPECT_FALSE(r.opened);
    EXPECT_FALSE(r.openedWithoutExist);
    EXPECT_EQ(r.entries, 0u);
}

TEST(ListDir, ARefusedListingIsNotAMissingFolder)
{
    TreeFileSystem fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/a.json", "{}", 1);
    fs.blockListingOutside(true);   // Gate 0: files open by name, folders will not list ...
    // ... except SharedData, which the firmware lets through; so block it by listing a sibling instead.
    fs.addDir("/Apps/Other");
    const ListResult r = listDir(fs, "../Other", nullptr, nullptr);
    EXPECT_FALSE(r.opened);
    EXPECT_EQ(r.entries, 0u);
}

TEST(ListDir, AFolderThatOpensAlthoughExistDeniedItIsRecordedAndNeverRead)
{
    DenyExistFs fs(pretend::kSandbox);
    fs.addFile("/Apps/SharedData/HybridX/a.json", "{}", 1);
    std::vector<ListEntry> seen;
    const ListResult r = listDir(fs, "../SharedData/HybridX", &collect, &seen);
    EXPECT_FALSE(r.exists);
    EXPECT_FALSE(r.opened);
    EXPECT_TRUE(r.openedWithoutExist);
    EXPECT_TRUE(seen.empty());
}

TEST(ListDir, StopsAtTheLimitAndSaysSo)
{
    TreeFileSystem fs(pretend::kSandbox);
    for (int i = 0; i < 10; ++i) {
        fs.addFile("/Apps/SharedData/f" + std::to_string(i), "x", 1);
    }
    const ListResult r = listDir(fs, "../SharedData", nullptr, nullptr, 4);
    EXPECT_EQ(r.entries, 4u);
    EXPECT_TRUE(r.full);
}
