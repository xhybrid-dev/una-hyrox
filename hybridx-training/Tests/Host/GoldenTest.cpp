// The nine plans Jon reviews at the TP1 gate (brief section 13), kept as
// golden files. A change to the plan files or the core that changes what the
// watch would build shows up here as a readable diff.
//
// To accept a change: XTRAINING_UPDATE_GOLDEN=1 hybridx-training-host-tests
// --gtest_filter=Golden.*, then read the diff before committing.

#include "PlanPrint.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

using namespace XTraining;

namespace {

struct Runner {
    const char* tag;
    uint32_t    fiveK;
    uint8_t     level;
    DayMask     days;
};

constexpr DayMask kFive = (1u << 1) | (1u << 2) | (1u << 3) | (1u << 5) | (1u << 6);  // Tue Wed Thu Sat Sun
constexpr DayMask kFour = WeekLayout::kDefaultDays;                                  // Tue Wed Fri Sun
constexpr DayMask kThree = (1u << 1) | (1u << 3) | (1u << 6);                        // Tue Thu Sun

constexpr Runner kRunners[] = {
    {"5k-20min-level3-5days", 20 * 60, 3, kFive},
    {"5k-25min-level2-4days", 25 * 60, 2, kFour},
    {"5k-32min-level1-3days", 32 * 60, 1, kThree},
};

struct Plan {
    const char* tag;
    PlanId      id;
};
constexpr Plan kPlans[] = {{"10k", PlanId::TenK}, {"half", PlanId::Half}, {"vo2max", PlanId::Vo2max}};

std::string readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace

TEST(Golden, TheNineGatePlans)
{
    const bool update = std::getenv("XTRAINING_UPDATE_GOLDEN") != nullptr;
    for (const Plan& p : kPlans) {
        for (const Runner& r : kRunners) {
            PrintInput in;
            in.plan = p.id;
            in.fiveKSec = r.fiveK;
            in.level = r.level;
            in.days = r.days;
            in.longDay = 6;
            const std::string text = printPlan(in);
            const std::string path = std::string(XTRAINING_GOLDEN_DIR) + "/" + p.tag + "-" + r.tag + ".md";
            if (update) {
                std::ofstream(path, std::ios::binary) << text;
                continue;
            }
            const std::string want = readFile(path);
            EXPECT_FALSE(want.empty()) << path << " is missing: run with XTRAINING_UPDATE_GOLDEN=1";
            EXPECT_EQ(text, want) << path << " differs: if the change is meant, run with XTRAINING_UPDATE_GOLDEN=1"
                                  << " and read the diff";
        }
    }
}

TEST(Golden, EveryPlanPrintsForEveryRunnerAndOption)
{
    // Not kept: a smoke test that nothing fails to place or build.
    for (const Plan& p : kPlans) {
        for (const Runner& r : kRunners) {
            for (int opt = 0; opt < 8; ++opt) {
                PrintInput in;
                in.plan = p.id;
                in.fiveKSec = r.fiveK;
                in.level = r.level;
                in.days = r.days;
                in.longDay = 6;
                in.fiveKRecent = !(opt & 1);
                in.easyByHr = opt & 2;
                in.repsByDistance = opt & 4;
                in.halfSec = opt & 1 ? r.fiveK * 5u : 0;
                const std::string text = printPlan(in);
                EXPECT_EQ(text.find("could not"), std::string::npos) << p.tag << " " << r.tag << " " << opt;
                EXPECT_NE(text.find("## The whole plan"), std::string::npos) << p.tag << " " << r.tag << " " << opt;
            }
        }
    }
}
