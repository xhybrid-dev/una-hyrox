/**
 * Host tests for the TreeFileSystem fake itself: the probe's and the
 * scanner's tests are only as good as the file system they run on.
 */

#include <gtest/gtest.h>

#include <set>
#include <string>

#include "TreeFileSystem.hpp"

namespace
{

std::set<std::string> listing(TreeFileSystem& fs, const char* path, bool* opened = nullptr)
{
    std::set<std::string> names;
    auto dir = fs.dir(path);
    const bool ok = dir && dir->open();
    if (opened) {
        *opened = ok;
    }
    if (ok) {
        SDK::Interface::IFileSystem::ObjectInfo info {};
        while (dir->readNext(info)) {
            names.insert(std::string(info.name) + (info.isDir ? "/" : ""));
        }
        dir->close();
    }
    return names;
}

} // namespace

TEST(TreeFileSystem, ResolvesRelativeParentAbsoluteAndDrivePaths)
{
    TreeFileSystem fs("/Apps/Me");
    EXPECT_EQ(fs.resolve("a.txt"), "/Apps/Me/a.txt");
    EXPECT_EQ(fs.resolve("./a.txt"), "/Apps/Me/a.txt");
    EXPECT_EQ(fs.resolve(".."), "/Apps");
    EXPECT_EQ(fs.resolve("../Running/Activity/"), "/Apps/Running/Activity");
    EXPECT_EQ(fs.resolve("/Apps//Other"), "/Apps/Other");
    EXPECT_EQ(fs.resolve("2:/Apps"), "/Apps");
    EXPECT_EQ(fs.resolve("../../../.."), "/");
}

TEST(TreeFileSystem, ListsSiblingsThroughParent)
{
    TreeFileSystem fs("/Apps/Me");
    fs.addFile("/Apps/Running/Activity/202609/activity_20260920T071500.fit", "x", 100);
    EXPECT_EQ(listing(fs, ".."), (std::set<std::string> { "Me/", "Running/" }));
    EXPECT_EQ(listing(fs, "../Running/Activity/202609"),
              (std::set<std::string> { "activity_20260920T071500.fit" }));

    SDK::Interface::IFileSystem::ObjectInfo info {};
    ASSERT_TRUE(fs.objectInfo("../Running/Activity/202609/activity_20260920T071500.fit", info));
    EXPECT_EQ(info.size, 1u);
    EXPECT_EQ(info.utc, 100);
}

TEST(TreeFileSystem, BlockParentAccessConfinesToSandboxAndSharedData)
{
    TreeFileSystem fs("/Apps/Me");
    fs.addFile("/Apps/Running/Activity/202609/a.fit", "x");
    fs.addDir("/Apps/SharedData");
    fs.blockParentAccess(true);

    bool opened = true;
    listing(fs, "..", &opened);
    EXPECT_FALSE(opened);
    EXPECT_FALSE(fs.exist("../Running/Activity/202609/a.fit"));
    EXPECT_FALSE(fs.file("../Running/Activity/202609/a.fit")->open());
    EXPECT_TRUE(fs.mkdir("../SharedData"));
    EXPECT_TRUE(fs.file("../SharedData/t.tmp")->open(true, true));
    EXPECT_TRUE(fs.file("mine.txt")->open(true, true));
}

TEST(TreeFileSystem, FilesWriteReadAppendAndMissingParentFails)
{
    TreeFileSystem fs("/Apps/Me");
    {
        auto f = fs.file("log.txt");
        ASSERT_TRUE(f->open(true, false));   // FA_OPEN_ALWAYS creates
        size_t n = 0;
        ASSERT_TRUE(f->write("one\n", 4, n));
        f->close();
    }
    {
        auto f = fs.file("log.txt");
        ASSERT_TRUE(f->open(true, false));
        ASSERT_TRUE(f->seek(f->size()));
        size_t n = 0;
        ASSERT_TRUE(f->write("two\n", 4, n));
        f->close();
    }
    EXPECT_EQ(fs.content("log.txt"), "one\ntwo\n");
    EXPECT_FALSE(fs.file("no/such/dir/x.txt")->open(true, true));
    EXPECT_FALSE(fs.file("absent.txt")->open(false, false));
}

TEST(TreeFileSystem, RenameRefusesExistingDestinationAndRemoveRefusesFullDirectory)
{
    TreeFileSystem fs("/Apps/Me");
    fs.addFile("a", "A");
    fs.addFile("b", "B");
    EXPECT_FALSE(fs.rename("a", "b"));
    EXPECT_EQ(fs.content("b"), "B");
    EXPECT_TRUE(fs.remove("b"));
    EXPECT_TRUE(fs.rename("a", "b"));
    EXPECT_EQ(fs.content("b"), "A");
    EXPECT_FALSE(fs.hasFile("a"));

    fs.addFile("dir/f", "x");
    EXPECT_FALSE(fs.remove("dir"));
    EXPECT_TRUE(fs.remove("dir/f"));
    EXPECT_TRUE(fs.remove("dir"));
}
