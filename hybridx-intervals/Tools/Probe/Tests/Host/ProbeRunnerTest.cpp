/**
 * Host tests for Probe::Runner (Tools/Probe), the P0 go/no-go check: does the
 * app find and read a file dropped into its own Plans/ folder?
 */

#include <gtest/gtest.h>

#include <cstring>

#include "ProbeRunner.hpp"
#include "support/FlatFileSystem.hpp"

using Probe::Check;
using Probe::Verdict;

namespace
{
class RecordingHost : public Probe::Host
{
public:
    void line(const char* text) override { lines.emplace_back(text); }
    std::vector<std::string> lines;

    bool said(const std::string& fragment) const
    {
        for (const auto& l : lines) {
            if (l.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};
} // namespace

TEST(ProbeRunner, EmptyFolderIsCreatedAndReportedEmpty)
{
    FlatFileSystem fs;
    RecordingHost  host;
    Probe::Runner  runner(fs, host);
    Probe::Result  r;
    runner.run(r);

    EXPECT_EQ(r.verdict, Verdict::Empty);
    EXPECT_EQ(r.folderOk, Check::Ok);
    EXPECT_EQ(r.fileCount, 0u);
    EXPECT_TRUE(host.said("nothing there yet"));
    // The folder now exists, for the BLE client's MKDIR to find already there.
    auto d = fs.dir("Plans");
    EXPECT_TRUE(d->open());
}

TEST(ProbeRunner, PreExistingPlansFolderIsFine)
{
    FlatFileSystem fs;
    fs.addDir("Plans");
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.folderOk, Check::Ok);
    EXPECT_EQ(r.verdict, Verdict::Empty);
}

TEST(ProbeRunner, OneFileIsFoundAndRead)
{
    FlatFileSystem fs;
    fs.addFile("Plans/plan_2026w09.json", "{\"probe\": true, \"sentAt\": \"2026-09-25T10:00:00Z\"}", 1758800000);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.fileCount, 1u);
    EXPECT_STREQ(r.newestName, "plan_2026w09.json");
    EXPECT_EQ(r.newestSize, 49u);
    EXPECT_EQ(r.newestUtc, 1758800000);
    EXPECT_STREQ(r.preview, "{\"probe\": true, \"sentAt\": \"2026-09-25T10:00:00Z\"}");
    EXPECT_TRUE(host.said("read 49"));
}

TEST(ProbeRunner, NewestFileByMtimeWinsRegardlessOfNameOrder)
{
    FlatFileSystem fs;
    fs.addFile("Plans/a-older.json", "older", 100);
    fs.addFile("Plans/z-newest.json", "newest content", 200);
    fs.addFile("Plans/m-middle.json", "middle", 150);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.fileCount, 3u);
    EXPECT_STREQ(r.newestName, "z-newest.json");
    EXPECT_STREQ(r.preview, "newest content");
}

TEST(ProbeRunner, EmptyFileIsGoWithAnEmptyPreview)
{
    FlatFileSystem fs;
    fs.addFile("Plans/empty.json", "", 100);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.newestSize, 0u);
    EXPECT_STREQ(r.preview, "");
}

TEST(ProbeRunner, LongContentIsTruncatedSafely)
{
    FlatFileSystem fs;
    fs.addFile("Plans/big.json", std::string(500, 'x'), 100);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.newestSize, 500u);
    EXPECT_EQ(std::strlen(r.preview), sizeof(r.preview) - 1);
}

TEST(ProbeRunner, NonPrintableBytesAreSanitised)
{
    // Built byte-by-byte: std::string::operator+=(const char*) stops at the
    // first '\0', so a string literal can't carry an embedded NUL.
    std::string binary = "start";
    for (unsigned char c : { 0x01, 0x02, 0x03, 0xFF, 0x00 }) {
        binary += static_cast<char>(c);
    }
    binary += "end";
    ASSERT_EQ(binary.size(), 13u);

    FlatFileSystem fs;
    fs.addFile("Plans/binary.dat", binary, 100);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_STREQ(r.preview, "start.....end") << "each of the 5 non-printable bytes becomes '.'";
}

TEST(ProbeRunner, SubdirectoriesInPlansAreIgnored)
{
    FlatFileSystem fs;
    fs.addDir("Plans/nested");
    fs.addFile("Plans/nested/inner.json", "should not be seen", 50);
    fs.addFile("Plans/real.json", "the only file", 100);
    RecordingHost host;
    Probe::Runner runner(fs, host);
    Probe::Result r;
    runner.run(r);

    EXPECT_EQ(r.fileCount, 1u);
    EXPECT_STREQ(r.newestName, "real.json");
}
