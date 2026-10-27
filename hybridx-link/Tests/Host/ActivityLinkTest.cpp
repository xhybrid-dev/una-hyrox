/**
 * The activity summary file: round trip, bounds, malformed input, the list
 * kept per app, time zones and the table of files.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

#include "ActivityLink.hpp"

using namespace HybridXLink;

namespace
{

constexpr uint32_t kBase = 1791000000u;   // October 2026

Entry entry(uint32_t startUtc, int16_t tz = 60, uint32_t dur = 2700)
{
    Entry e;
    e.startUtc    = startUtc;
    e.tzOffsetMin = tz;
    e.durationSec = dur;
    e.distanceM   = 5400;
    e.avgHr       = 152;
    e.maxHr       = 171;
    e.sport       = Sport::Run;
    return e;
}

std::string write(const ActivityFile& f)
{
    char buf[kMaxBytes];
    const size_t len = encode(f, buf, sizeof(buf));
    return std::string(buf, len);
}

bool read(const std::string& text, ActivityFile& out)
{
    return decode(text.c_str(), text.size(), out);
}

void setApp(ActivityFile& f, const char* app)
{
    std::snprintf(f.app, sizeof(f.app), "%s", app);
}

ActivityFile fileOf(const char* app, std::initializer_list<Entry> entries)
{
    ActivityFile f;
    setApp(f, app);
    for (const Entry& e : entries) {
        f.entries[f.count++] = e;
    }
    return f;
}

}  // namespace

TEST(ActivityLink, RoundTrip)
{
    Entry second = entry(kBase + 86400, -480, 1800);
    second.sport    = Sport::Ride;
    second.distanceM = 0;
    second.avgHr = 0;
    second.maxHr = 0;
    const ActivityFile in = fileOf("HybridXRun", {entry(kBase), second});

    const std::string text = write(in);
    ASSERT_FALSE(text.empty());
    ActivityFile out;
    ASSERT_TRUE(read(text, out));
    EXPECT_STREQ(out.app, "HybridXRun");
    ASSERT_EQ(out.count, 2);
    EXPECT_EQ(out.skipped, 0);
    EXPECT_TRUE(out.entries[0] == in.entries[0]);
    EXPECT_TRUE(out.entries[1] == in.entries[1]);
}

TEST(ActivityLink, TheFileIsTheDocumentedShape)
{
    const std::string text = write(fileOf("HybridXRun", {entry(1791980037u, 60, 2700)}));
    EXPECT_EQ(text, R"({"v":1,"app":"HybridXRun","a":[[1791980037,1500,2700,5400,152,171,1]]})");
}

TEST(ActivityLink, AnEmptyListIsAFile)
{
    const std::string text = write(fileOf("HybridXRun", {}));
    ASSERT_FALSE(text.empty());
    ActivityFile out;
    ASSERT_TRUE(read(text, out));
    EXPECT_EQ(out.count, 0);
}

TEST(ActivityLink, TheWorstCaseFits)
{
    ActivityFile f;
    setApp(f, "abcdefghijklmno");   // 15 characters
    for (unsigned i = 0; i < kMaxEntries; ++i) {
        Entry e;
        e.startUtc    = 4294967295u - (kMaxEntries - i);   // ten digits
        e.tzOffsetMin = kMaxTzOffsetMin;
        e.durationSec = kMaxDurationSec;
        e.distanceM   = kMaxDistanceM;
        e.avgHr       = 255;
        e.maxHr       = 255;
        e.sport       = Sport::Other;
        f.entries[f.count++] = e;
    }
    char buf[kMaxBytes];
    const size_t len = encode(f, buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_LT(len, kMaxBytes);
    EXPECT_LT(len, 800u);   // the header comment says "about 710"

    ActivityFile out;
    ASSERT_TRUE(decode(buf, len, out));
    EXPECT_EQ(out.count, kMaxEntries);
}

TEST(ActivityLink, TooSmallABufferWritesNothing)
{
    const ActivityFile f = fileOf("HybridXRun", {entry(kBase)});
    char tiny[16] = "x";
    EXPECT_EQ(encode(f, tiny, sizeof(tiny)), 0u);
    EXPECT_EQ(tiny[0], '\0');
    EXPECT_EQ(encode(f, nullptr, 100), 0u);
    char none[1] = {'x'};
    EXPECT_EQ(encode(f, none, 0), 0u);
}

TEST(ActivityLink, EncodeRefusesWhatTheReaderWouldRefuse)
{
    char buf[kMaxBytes];

    Entry tooOld = entry(kMinStartUtc - 1);
    EXPECT_EQ(encode(fileOf("HybridXRun", {tooOld}), buf, sizeof(buf)), 0u);

    Entry hrBackwards = entry(kBase);
    hrBackwards.avgHr = 180;
    hrBackwards.maxHr = 170;
    EXPECT_EQ(encode(fileOf("HybridXRun", {hrBackwards}), buf, sizeof(buf)), 0u);

    EXPECT_EQ(encode(fileOf("", {entry(kBase)}), buf, sizeof(buf)), 0u);
    EXPECT_EQ(encode(fileOf("has space", {entry(kBase)}), buf, sizeof(buf)), 0u);
}

TEST(ActivityLink, MalformedFilesReadAsNothing)
{
    const char* const cases[] = {
        "",
        "not json",
        "[]",
        "{}",
        R"({"v":1})",
        R"({"v":1,"app":"HybridXRun"})",
        R"({"app":"HybridXRun","a":[]})",
        R"({"v":2,"app":"HybridXRun","a":[]})",                       // a newer version
        R"({"v":0,"app":"HybridXRun","a":[]})",
        R"({"v":1,"app":"","a":[]})",
        R"({"v":1,"app":"AVeryLongAppFolderName","a":[]})",           // over 15 characters
        R"({"v":1,"app":"Has Space","a":[]})",
        R"({"v":1,"app":"a/b","a":[]})",
        R"({"v":1,"app":"..","a":[]})",
        R"({"v":1,"app":"HybridXRun","a":{}})",
        R"({"v":1,"app":"HybridXRun","a":[[1791000000,1500,2700,5400,152,171,1])",   // truncated
    };
    for (const char* c : cases) {
        ActivityFile out;
        EXPECT_FALSE(decode(c, std::strlen(c), out)) << c;
    }
    ActivityFile out;
    EXPECT_FALSE(decode(nullptr, 5, out));
}

TEST(ActivityLink, TooManyEntriesReadsAsNothing)
{
    std::string text = R"({"v":1,"app":"HybridXRun","a":[)";
    for (unsigned i = 0; i < kMaxEntries + 1u; ++i) {
        char one[64];
        std::snprintf(one, sizeof(one), "%s[%u,1500,2700,0,0,0,1]", i ? "," : "", kBase + i);
        text += one;
    }
    text += "]}";
    ActivityFile out;
    EXPECT_FALSE(read(text, out));
}

TEST(ActivityLink, ABadEntryIsSkippedAndTheRestKept)
{
    // Entries: good; time before 2020; zone out of range; negative duration; short row;
    // average above maximum; distance over the limit; a string; good.
    const std::string text =
        R"({"v":1,"app":"HybridXRun","a":[)"
        "[1791000000,1500,2700,5400,152,171,1],"
        "[1500000000,1500,2700,5400,152,171,1],"
        "[1791000100,3000,2700,5400,152,171,1],"
        "[1791000200,1500,-5,5400,152,171,1],"
        "[1791000300,1500,2700],"
        "[1791000400,1500,2700,5400,180,170,1],"
        "[1791000500,1500,2700,1000001,152,171,1],"
        R"(["x",1500,2700,5400,152,171,1],)"
        "[1791000600,1500,1800,0,0,0,2]"
        "]}";
    ActivityFile out;
    ASSERT_TRUE(read(text, out));
    ASSERT_EQ(out.count, 2);
    EXPECT_EQ(out.skipped, 7);
    EXPECT_EQ(out.entries[0].startUtc, 1791000000u);
    EXPECT_EQ(out.entries[1].startUtc, 1791000600u);
    EXPECT_EQ(out.entries[1].sport, Sport::Ride);
}

TEST(ActivityLink, AnUnknownSportReadsAsOther)
{
    const std::string text = R"({"v":1,"app":"HybridXRun","a":[[1791000000,1500,2700,0,0,0,200],[1791000100,1500,2700,0,0,0,0]]})";
    ActivityFile out;
    ASSERT_TRUE(read(text, out));
    ASSERT_EQ(out.count, 2);
    EXPECT_EQ(out.entries[0].sport, Sport::Other);
    EXPECT_EQ(out.entries[1].sport, Sport::Other);
    EXPECT_EQ(sportFromCode(1), Sport::Run);
    EXPECT_EQ(sportFromCode(6), Sport::Hybrid);
    EXPECT_EQ(sportFromCode(8), Sport::Other);
    EXPECT_EQ(sportFromCode(-1), Sport::Other);
}

// ---- time zones --------------------------------------------------------------------------

TEST(ActivityLink, ZonesGoThroughTheFileWithoutANegativeNumber)
{
    const int16_t zones[] = {-720, -480, 0, 60, 330, 765, 840};
    for (int16_t z : zones) {
        const std::string text = write(fileOf("HybridXRun", {entry(kBase, z)}));
        ASSERT_FALSE(text.empty()) << z;
        EXPECT_EQ(text.find('-', text.find("\"a\"")), std::string::npos) << text;
        ActivityFile out;
        ASSERT_TRUE(read(text, out)) << z;
        EXPECT_EQ(out.entries[0].tzOffsetMin, z);
    }
}

TEST(ActivityLink, LocalStartLandsOnTheLocalDay)
{
    // 2026-10-27 23:30 UTC is already the 28th at UTC+13 and still the 27th at UTC-8.
    const uint32_t utc = 1793143800u;
    ASSERT_EQ(utc % 86400u, 23u * 3600u + 30u * 60u);   // 23:30 on its UTC day

    const uint32_t utcDay = utc / 86400u;
    EXPECT_EQ(entry(utc, 0).localStart() / 86400u, utcDay);
    EXPECT_EQ(entry(utc, 13 * 60).localStart() / 86400u, utcDay + 1);
    EXPECT_EQ(entry(utc, -8 * 60).localStart() / 86400u, utcDay);
    // And early in the UTC day it is the day before to the west.
    const uint32_t early = utc - 22u * 3600u;   // 01:30 UTC
    EXPECT_EQ(entry(early, -8 * 60).localStart() / 86400u, utcDay - 1);
    EXPECT_EQ(entry(utc, 60).localStart(), utc + 3600u);
}

TEST(ActivityLink, TzOffsetFromALocalClock)
{
    const uint32_t utc = kBase;
    EXPECT_EQ(tzOffsetMinutes(utc, utc), 0);
    EXPECT_EQ(tzOffsetMinutes(utc, utc + 3600), 60);
    EXPECT_EQ(tzOffsetMinutes(utc, static_cast<int64_t>(utc) - 8 * 3600), -480);
    EXPECT_EQ(tzOffsetMinutes(utc, utc + 19800), 330);          // India
    EXPECT_EQ(tzOffsetMinutes(utc, utc + 3600 + 25), 60);       // a few seconds of drift rounds away
    EXPECT_EQ(tzOffsetMinutes(utc, utc + 3600 + 35), 61);
    EXPECT_EQ(tzOffsetMinutes(utc, static_cast<int64_t>(utc) - 3600 - 35), -61);
    EXPECT_EQ(tzOffsetMinutes(utc, utc + 20 * 3600), kMaxTzOffsetMin);   // nonsense is clamped
    EXPECT_EQ(tzOffsetMinutes(utc, static_cast<int64_t>(utc) - 20 * 3600), kMinTzOffsetMin);
}

// ---- adding an activity ------------------------------------------------------------------

TEST(ActivityLinkAppend, TheFirstActivityMakesTheFile)
{
    char buf[kMaxBytes];
    const size_t len = append(nullptr, 0, "HybridXRun", entry(kBase), buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    ActivityFile out;
    ASSERT_TRUE(decode(buf, len, out));
    EXPECT_STREQ(out.app, "HybridXRun");
    ASSERT_EQ(out.count, 1);
    EXPECT_TRUE(out.entries[0] == entry(kBase));
}

TEST(ActivityLinkAppend, ActivitiesBuildUpOldestFirst)
{
    char a[kMaxBytes], b[kMaxBytes];
    size_t la = append(nullptr, 0, "HybridXRun", entry(kBase), a, sizeof(a));
    size_t lb = append(a, la, "HybridXRun", entry(kBase + 1000), b, sizeof(b));
    la        = append(b, lb, "HybridXRun", entry(kBase + 2000), a, sizeof(a));
    ActivityFile out;
    ASSERT_TRUE(decode(a, la, out));
    ASSERT_EQ(out.count, 3);
    EXPECT_EQ(out.entries[0].startUtc, kBase);
    EXPECT_EQ(out.entries[1].startUtc, kBase + 1000);
    EXPECT_EQ(out.entries[2].startUtc, kBase + 2000);
}

TEST(ActivityLinkAppend, AnEarlierStartGoesBeforeALaterOne)
{
    char a[kMaxBytes], b[kMaxBytes];
    const size_t la = append(nullptr, 0, "HybridXRun", entry(kBase + 5000), a, sizeof(a));
    const size_t lb = append(a, la, "HybridXRun", entry(kBase), b, sizeof(b));
    ActivityFile out;
    ASSERT_TRUE(decode(b, lb, out));
    ASSERT_EQ(out.count, 2);
    EXPECT_EQ(out.entries[0].startUtc, kBase);
    EXPECT_EQ(out.entries[1].startUtc, kBase + 5000);
}

TEST(ActivityLinkAppend, TheSameStartTwiceIsOneActivity)
{
    char a[kMaxBytes], b[kMaxBytes];
    const size_t la = append(nullptr, 0, "HybridXRun", entry(kBase, 60, 1000), a, sizeof(a));
    const size_t lb = append(a, la, "HybridXRun", entry(kBase, 60, 2000), b, sizeof(b));
    ActivityFile out;
    ASSERT_TRUE(decode(b, lb, out));
    ASSERT_EQ(out.count, 1);
    EXPECT_EQ(out.entries[0].durationSec, 2000u);   // the later write wins
}

TEST(ActivityLinkAppend, OnlyTheNewestSixteenAreKept)
{
    char a[kMaxBytes], b[kMaxBytes];
    size_t len = 0;
    const char* prev = nullptr;
    char* bufs[2] = {a, b};
    for (unsigned i = 0; i < 20; ++i) {
        char* to = bufs[i % 2];
        len      = append(prev, prev ? len : 0, "HybridXRun", entry(kBase + i * 1000), to, kMaxBytes);
        ASSERT_GT(len, 0u) << i;
        prev = to;
    }
    ActivityFile out;
    ASSERT_TRUE(decode(prev, len, out));
    ASSERT_EQ(out.count, kMaxEntries);
    EXPECT_EQ(out.entries[0].startUtc, kBase + 4000u);    // activities 0 to 3 dropped
    EXPECT_EQ(out.entries[15].startUtc, kBase + 19000u);
}

TEST(ActivityLinkAppend, AFullFileDropsAnActivityOlderThanAllOfThem)
{
    char a[kMaxBytes], b[kMaxBytes];
    ActivityFile f;
    setApp(f, "HybridXRun");
    for (unsigned i = 0; i < kMaxEntries; ++i) {
        f.entries[f.count++] = entry(kBase + 10000 + i * 1000);
    }
    const size_t la = encode(f, a, sizeof(a));
    ASSERT_GT(la, 0u);
    const size_t lb = append(a, la, "HybridXRun", entry(kBase), b, sizeof(b));
    ActivityFile out;
    ASSERT_TRUE(decode(b, lb, out));
    ASSERT_EQ(out.count, kMaxEntries);
    EXPECT_EQ(out.entries[0].startUtc, kBase + 10000u);
}

TEST(ActivityLinkAppend, AFullFileInsertsInTheMiddle)
{
    char a[kMaxBytes], b[kMaxBytes];
    ActivityFile f;
    setApp(f, "HybridXRun");
    for (unsigned i = 0; i < kMaxEntries; ++i) {
        f.entries[f.count++] = entry(kBase + i * 1000);
    }
    const size_t la = encode(f, a, sizeof(a));
    const size_t lb = append(a, la, "HybridXRun", entry(kBase + 7500), b, sizeof(b));
    ActivityFile out;
    ASSERT_TRUE(decode(b, lb, out));
    ASSERT_EQ(out.count, kMaxEntries);
    EXPECT_EQ(out.entries[0].startUtc, kBase + 1000u);     // the oldest went
    EXPECT_EQ(out.entries[7].startUtc, kBase + 7500u);     // the new one sits between 7000 and 8000
    EXPECT_EQ(out.entries[6].startUtc, kBase + 7000u);
    EXPECT_EQ(out.entries[8].startUtc, kBase + 8000u);
    EXPECT_EQ(out.entries[15].startUtc, kBase + 15000u);
}

TEST(ActivityLinkAppend, AFileThatDoesNotReadStartsFresh)
{
    const char* const bad[] = {"garbage", "{\"v\":9}", "", R"({"v":1,"app":"HybridXRun","a":[[1,2]]})"};
    for (const char* text : bad) {
        char buf[kMaxBytes];
        const size_t len = append(text, std::strlen(text), "HybridXRun", entry(kBase), buf, sizeof(buf));
        ASSERT_GT(len, 0u) << text;
        ActivityFile out;
        ASSERT_TRUE(decode(buf, len, out)) << text;
        EXPECT_EQ(out.count, 1) << text;
    }
}

TEST(ActivityLinkAppend, AnotherAppsFileIsNotExtended)
{
    char a[kMaxBytes], b[kMaxBytes];
    const size_t la = append(nullptr, 0, "HybridXRace", entry(kBase), a, sizeof(a));
    const size_t lb = append(a, la, "HybridXRun", entry(kBase + 1000), b, sizeof(b));
    ActivityFile out;
    ASSERT_TRUE(decode(b, lb, out));
    EXPECT_STREQ(out.app, "HybridXRun");
    EXPECT_EQ(out.count, 1);
}

TEST(ActivityLinkAppend, BadArgumentsWriteNothing)
{
    char buf[kMaxBytes] = "x";
    EXPECT_EQ(append(nullptr, 0, nullptr, entry(kBase), buf, sizeof(buf)), 0u);
    EXPECT_EQ(buf[0], '\0');
    EXPECT_EQ(append(nullptr, 0, "no good", entry(kBase), buf, sizeof(buf)), 0u);
    EXPECT_EQ(append(nullptr, 0, "HybridXRun", entry(0), buf, sizeof(buf)), 0u);
    char tiny[8];
    EXPECT_EQ(append(nullptr, 0, "HybridXRun", entry(kBase), tiny, sizeof(tiny)), 0u);
    EXPECT_EQ(tiny[0], '\0');
}

// ---- the table of files ------------------------------------------------------------------

TEST(ActivityLinkSlots, EveryAppHasItsOwnFileInTheSharedFolder)
{
    EXPECT_LE(kSlotCount, kMaxSlots);
    std::set<std::string> seen;
    const std::string dir = std::string(kSharedDir) + "/";
    for (uint8_t i = 0; i < kSlotCount; ++i) {
        const std::string p = kSlotPaths[i];
        EXPECT_EQ(p.compare(0, dir.size(), dir), 0) << p;
        EXPECT_NE(p.find("act_"), std::string::npos) << p;
        EXPECT_EQ(p.substr(p.size() - 5), ".json") << p;
        // A file's name must leave room for ".tmp" and ".bak" and stay a short FAT name.
        EXPECT_LT(p.size() - dir.size(), 32u) << p;
        EXPECT_TRUE(seen.insert(p).second) << "duplicate " << p;
    }
}

TEST(ActivityLinkSlots, SlotPathFollowsTheEnum)
{
    EXPECT_STREQ(slotPath(Slot::Run), "../SharedData/HybridX/act_run.json");
    EXPECT_STREQ(slotPath(Slot::Workout), "../SharedData/HybridX/act_workout.json");
    EXPECT_EQ(slotPath(static_cast<Slot>(kSlotCount)), nullptr);
    EXPECT_EQ(static_cast<uint8_t>(Slot::Workout) + 1, kSlotCount);
}
