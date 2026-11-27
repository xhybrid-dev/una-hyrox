#include <gtest/gtest.h>

#include <cstring>

#include "ActivityLink.hpp"
#include "KnownFiles.hpp"
#include "StreakNow.hpp"

using namespace ClockProbe;

TEST(KnownFiles, TheActivityFilesAreTheLinksOwnPaths)
{
    static_assert(kLinkFiles == HybridXLink::kSlotCount, "the probe must know every slot the link has");
    for (uint8_t i = 0; i < kLinkFiles; ++i) {
        EXPECT_STREQ(kKnown[i].path, HybridXLink::kSlotPaths[i]) << "slot " << int(i);
        EXPECT_EQ(kKnown[i].role, Role::Link);
    }
}

TEST(KnownFiles, TheScreensFilesAreWhereTheIndexesSay)
{
    EXPECT_STREQ(kKnown[kIdxStreakNow].label, "streak_now");
    EXPECT_EQ(kKnown[kIdxStreakNow].role, Role::StreakNow);
    EXPECT_STREQ(kKnown[kIdxToday].label, "training_today");
    EXPECT_EQ(kKnown[kIdxToday].role, Role::TrainingToday);
    EXPECT_STREQ(kKnown[kIdxVo2].label, "vo2max");
    EXPECT_EQ(kKnown[kIdxVo2].role, Role::Vo2Max);
}

TEST(KnownFiles, EveryPathIsInTheSharedFolderAndFitsTheLimit)
{
    for (const Known& k : kKnown) {
        EXPECT_EQ(std::strncmp(k.path, "../SharedData/", 14), 0) << k.path;
        EXPECT_LT(std::strlen(k.path), 256u) << k.path;
        EXPECT_LE(std::strlen(k.label), 15u) << k.label;
    }
}

TEST(KnownFiles, NothingPrivateIsEverOpened)
{
    // X Weigh-in's log, Streak's own state and the SDK's stride file are asked about, never read.
    for (const Known& k : kKnown) {
        const bool priv = std::strstr(k.path, "weighins") || std::strstr(k.path, "stride") ||
                          std::strstr(k.path, "/streak.json");
        if (priv) {
            EXPECT_EQ(k.role, Role::SizeOnly) << k.path;
        }
    }
}

TEST(KnownFiles, TheScratchBufferHoldsTheBiggestFileTheFamilyWrites)
{
    EXPECT_GT(kScratchBytes, HybridXLink::kMaxBytes);
    EXPECT_GT(kScratchBytes, HybridXLink::kStreakNowMaxBytes);
}
