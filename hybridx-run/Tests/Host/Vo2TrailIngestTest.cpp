#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "Vo2TrailIngest.hpp"

using namespace RunVo2;

namespace
{

Profile profile(uint8_t maxHr = 190, uint8_t restHr = 50)
{
    Profile p;
    p.status = ProfileStatus::Ok;
    p.maxHr  = maxHr;
    p.restHr = restHr;
    return p;
}

WindowRecord goodWindow(uint32_t activeSec)
{
    WindowRecord w;
    w.activeSec    = activeSec;
    w.goodSec      = 60;
    w.speedMMinX10 = 2000;   // 200 m/min, HR 160, rest 50, max 190: 54.4
    w.spreadX10    = 20;
    w.hrX10        = 1600;
    w.hrMin        = 158;
    w.hrMax        = 162;
    return w;
}

size_t fileOf(char* buf, uint32_t id, uint8_t maxHr, int windows)
{
    static WindowRecord w[40];
    for (int i = 0; i < windows; ++i) {
        w[i] = goodWindow(360 + 60 * i);
    }
    return writeWindowFile(buf, kWindowFileBytes, id, maxHr, w, static_cast<uint16_t>(windows));
}

}  // namespace

TEST(Vo2TrailIngest, ANewRunIsJudgedWithRunsOwnRules)
{
    char buf[kWindowFileBytes];
    const size_t n = fileOf(buf, 1791980037u, 178, 10);
    ASSERT_GT(n, 0u);

    Vo2Run run;
    WindowFileInfo info;
    ASSERT_EQ(judgeWindowFile(buf, n, 0, run, info), IngestStatus::Judged);
    EXPECT_EQ(info.runId, 1791980037u);
    EXPECT_EQ(info.count, 10);
    EXPECT_EQ(run.counts().accepted, 10);
    EXPECT_EQ(run.sustainedMaxHr(), 178);
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.vo2x10, 544);
    EXPECT_EQ(r.windowsUsed, 10);
}

TEST(Vo2TrailIngest, TheSameProfileScoresTheSameWindowsDifferently)
{
    // The profile is Run's, applied on reading: a higher max HR, a higher figure.
    char buf[kWindowFileBytes];
    const size_t n = fileOf(buf, 1791980037u, 0, 10);
    Vo2Run run;
    WindowFileInfo info;
    ASSERT_EQ(judgeWindowFile(buf, n, 0, run, info), IngestStatus::Judged);
    EXPECT_GT(run.estimate(profile(200)).vo2x10, run.estimate(profile(190)).vo2x10);
}

TEST(Vo2TrailIngest, ARunAtOrBeforeTheMarkIsNotCountedAgain)
{
    char buf[kWindowFileBytes];
    const size_t n = fileOf(buf, 1791980037u, 178, 10);
    Vo2Run run;
    WindowFileInfo info;
    EXPECT_EQ(judgeWindowFile(buf, n, 1791980037u, run, info), IngestStatus::AlreadyCounted);
    EXPECT_EQ(info.runId, 1791980037u);   // known, so the service can log it
    EXPECT_EQ(judgeWindowFile(buf, n, 1791980038u, run, info), IngestStatus::AlreadyCounted);
    EXPECT_EQ(judgeWindowFile(buf, n, 1791980036u, run, info), IngestStatus::Judged);
}

TEST(Vo2TrailIngest, UnreadableFilesCountNothing)
{
    Vo2Run run;
    WindowFileInfo info;
    EXPECT_EQ(judgeWindowFile(nullptr, 0, 0, run, info), IngestStatus::NotAFile);
    EXPECT_EQ(judgeWindowFile("", 0, 0, run, info), IngestStatus::NotAFile);
    const char* junk = "{\"hello\":1}";
    EXPECT_EQ(judgeWindowFile(junk, std::strlen(junk), 0, run, info), IngestStatus::NotAFile);
    EXPECT_EQ(info.runId, 0u);
    const char* newer = "{\"v\":2,\"id\":5,\"max\":0,\"w\":[]}";
    EXPECT_EQ(judgeWindowFile(newer, std::strlen(newer), 0, run, info), IngestStatus::NotAFile);
}

TEST(Vo2TrailIngest, ABadWindowMakesTheWholeRunCorrupt)
{
    // A good header and id (so the service can mark it and never retry it),
    // but the second window has 99 good seconds.
    const char* t = "{\"v\":1,\"id\":1791980037,\"max\":170,\"w\":["
                    "[420,60,2000,20,3000,0,1600,158,162],"
                    "[480,99,2000,20,3000,0,1600,158,162]]}";
    Vo2Run run;
    WindowFileInfo info;
    EXPECT_EQ(judgeWindowFile(t, std::strlen(t), 0, run, info), IngestStatus::Corrupt);
    EXPECT_EQ(info.runId, 1791980037u);
    EXPECT_EQ(run.counts().accepted, 0);          // nothing left behind
    EXPECT_EQ(run.sustainedMaxHr(), 0);
}

TEST(Vo2TrailIngest, ARunWithTooFewSteadyWindowsHasNoEstimate)
{
    char buf[kWindowFileBytes];
    const size_t n = fileOf(buf, 1791980037u, 170, 3);
    Vo2Run run;
    WindowFileInfo info;
    ASSERT_EQ(judgeWindowFile(buf, n, 0, run, info), IngestStatus::Judged);
    EXPECT_EQ(run.estimate(profile()).status, RunStatus::NotEnoughRunning);
}

TEST(Vo2TrailIngest, JudgingOneFileForgetsTheLast)
{
    char a[kWindowFileBytes], b[kWindowFileBytes];
    const size_t na = fileOf(a, 100, 175, 10);
    const size_t nb = fileOf(b, 200, 0, 6);
    Vo2Run run;
    WindowFileInfo info;
    ASSERT_EQ(judgeWindowFile(a, na, 0, run, info), IngestStatus::Judged);
    ASSERT_EQ(judgeWindowFile(b, nb, 0, run, info), IngestStatus::Judged);
    EXPECT_EQ(run.counts().accepted, 6);
    EXPECT_EQ(run.sustainedMaxHr(), 0);
}

TEST(Vo2TrailIngest, SlotsAreTakenOldestFirstAboveTheMark)
{
    const uint32_t ids[3] = {300, 100, 200};
    EXPECT_EQ(nextWindowFileSlot(ids, 3, 0), 1);
    EXPECT_EQ(nextWindowFileSlot(ids, 3, 100), 2);
    EXPECT_EQ(nextWindowFileSlot(ids, 3, 200), 0);
    EXPECT_EQ(nextWindowFileSlot(ids, 3, 300), -1);
    const uint32_t some[3] = {0, 250, 0};   // empty slots are skipped
    EXPECT_EQ(nextWindowFileSlot(some, 3, 0), 1);
    EXPECT_EQ(nextWindowFileSlot(some, 3, 250), -1);
    const uint32_t same[3] = {200, 200, 0}; // a copy in two slots: once, then the mark covers it
    EXPECT_EQ(nextWindowFileSlot(same, 3, 0), 0);
    EXPECT_EQ(nextWindowFileSlot(same, 3, 200), -1);
}

// ---- X Training's runs (its TP4): the same files and rules, its own names and mark --------

TEST(Vo2TrainingIngest, XTrainingsFilesAreItsOwnInTheSharedFolder)
{
    for (uint8_t i = 0; i < kWindowFileSlots; ++i) {
        const std::string t = kTrainingWindowFilePaths[i];
        EXPECT_EQ(t.rfind("../SharedData/HybridX/training_vo2_", 0), 0u) << t;
        for (uint8_t j = 0; j < kWindowFileSlots; ++j) {
            EXPECT_STRNE(kTrainingWindowFilePaths[i], kWindowFilePaths[j]);   // one writer per file
        }
    }
}

TEST(Vo2TrainingIngest, EachSourcesMarkHidesOnlyItsOwnRuns)
{
    // Trail's newest run (300) is newer than X Training's two (150, 250). Behind one shared mark
    // they would never count; behind their own (0), both do, oldest first, then not again.
    const uint32_t trail[kWindowFileSlots]    = {100, 200, 300};
    const uint32_t training[kWindowFileSlots] = {250, 0, 150};
    const uint32_t trailMark = 300, trainingMark = 0;
    EXPECT_EQ(nextWindowFileSlot(trail, kWindowFileSlots, trailMark), -1);
    EXPECT_EQ(nextWindowFileSlot(training, kWindowFileSlots, trailMark), -1);   // what one mark would do
    EXPECT_EQ(nextWindowFileSlot(training, kWindowFileSlots, trainingMark), 2);   // 150 first
    EXPECT_EQ(nextWindowFileSlot(training, kWindowFileSlots, 150), 0);            // then 250
    EXPECT_EQ(nextWindowFileSlot(training, kWindowFileSlots, 250), -1);           // then none

    char buf[kWindowFileBytes];
    const size_t n = fileOf(buf, 150, 176, 12);
    Vo2Run run;
    WindowFileInfo info;
    ASSERT_EQ(judgeWindowFile(buf, n, trainingMark, run, info), IngestStatus::Judged);
    EXPECT_EQ(run.estimate(profile()).vo2x10, 544);   // judged exactly as a Trail run
    EXPECT_EQ(judgeWindowFile(buf, n, 150, run, info), IngestStatus::AlreadyCounted);
}
