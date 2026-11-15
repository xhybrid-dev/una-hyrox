#include "PlanState.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using namespace XTraining;

namespace {

PlanState sample()
{
    PlanState s;
    s.setup.plan = PlanId::Half;
    s.setup.level = 2;
    s.setup.raceDate = 20270307u;
    s.setup.fiveKSec = 22 * 60;
    s.setup.fiveKDate = 20261115u;
    s.setup.halfSec = 3600 + 45 * 60;
    s.setup.halfDate = 20261115u;
    s.calendar = PlanCalendar::make({20261115u, s.setup.raceDate, 12, 8});
    s.record({20261215u, Kind::Cruise, DayStatus::Done, 1797314400u});
    s.record({20261216u, Kind::Easy, DayStatus::Missed, 0});
    s.record({20261218u, Kind::EasyStrides, DayStatus::PartDone, 1797487200u});
    return s;
}

std::string json(const PlanState& s)
{
    std::vector<char> buf(PlanState::kMaxJsonBytes);
    const size_t n = s.toJson(buf.data(), buf.size());
    return std::string(buf.data(), n);
}

}  // namespace

TEST(PlanState, RoundTrip)
{
    const PlanState s = sample();
    const std::string text = json(s);
    ASSERT_FALSE(text.empty());
    EXPECT_EQ(text,
              "{\"v\":1,\"s\":[2,2,86,6,0,0,20270307,1320,20261115,1,6300,20261115],"
              "\"c\":[0,20261214,0,12],"
              "\"log\":[[20261215,6,1,1797314400],[20261216,1,3,0],[20261218,2,2,1797487200]]}");
    PlanState back;
    ASSERT_TRUE(back.fromJson(text.data(), text.size()));
    EXPECT_EQ(json(back), text);
    EXPECT_TRUE(back.active());
    ASSERT_NE(back.find(20261216u), nullptr);
    EXPECT_EQ(back.find(20261216u)->status, DayStatus::Missed);
    EXPECT_EQ(back.find(20261217u), nullptr);
}

TEST(PlanState, RecordReplacesAndKeepsOrder)
{
    PlanState s;
    s.record({20261218u, Kind::Easy, DayStatus::Missed, 0});
    s.record({20261216u, Kind::Easy, DayStatus::Done, 5});
    s.record({20261218u, Kind::Easy, DayStatus::Done, 7});
    ASSERT_EQ(s.logCount(), 2);
    EXPECT_EQ(s.log(0).date, 20261216u);
    EXPECT_EQ(s.log(1).status, DayStatus::Done);
}

TEST(PlanState, FullLogDropsTheOldest)
{
    PlanState s;
    for (int i = 0; i < 100; ++i) {
        s.record({Date::addDays(20261116u, i), Kind::Easy, DayStatus::Done, 0});
    }
    EXPECT_EQ(s.logCount(), PlanState::kMaxLog);
    EXPECT_EQ(s.log(0).date, Date::addDays(20261116u, 100 - PlanState::kMaxLog));
    // The largest file still fits.
    s.setup.plan = PlanId::TenK;
    s.setup.fiveKSec = 59 * 60 + 59;
    const std::string text = json(s);
    ASSERT_FALSE(text.empty());
    EXPECT_LT(text.size(), PlanState::kMaxJsonBytes);
}

TEST(PlanState, EveryPrefixAndOneByteChangeIsSafe)
{
    const std::string text = json(sample());
    PlanState s;
    for (size_t n = 0; n < text.size(); ++n) {
        std::vector<char> exact(text.begin(), text.begin() + static_cast<long>(n));
        s.fromJson(exact.data(), exact.size());  // must not read past the end (run under ASan)
    }
    int accepted = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        for (char c : {'0', '9', '-', ']', '[', ',', '"', ' ', 'x'}) {
            std::string t = text;
            if (t[i] == c) {
                continue;
            }
            t[i] = c;
            PlanState x;
            if (x.fromJson(t.data(), t.size())) {
                ++accepted;
                // A change that still reads must still be a sane state.
                EXPECT_TRUE(x.setup.level >= 1 && x.setup.level <= 3);
                EXPECT_LE(x.logCount(), PlanState::kMaxLog);
            } else {
                EXPECT_EQ(x.setup.plan, PlanId::None);
                EXPECT_EQ(x.logCount(), 0);
            }
        }
    }
    EXPECT_GT(accepted, 0);  // some digit changes are still valid states
}

TEST(PlanState, RefusesBadValues)
{
    const char* bad[] = {
        "{}",
        "{\"v\":2,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[]}",
        // level 4
        "{\"v\":1,\"s\":[2,4,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[]}",
        // a 5K of 5 minutes
        "{\"v\":1,\"s\":[2,2,86,6,0,0,0,300,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[]}",
        // the long day not a run day
        "{\"v\":1,\"s\":[2,2,86,0,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[]}",
        // a start that is not a Monday
        "{\"v\":1,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261215,0,12],\"log\":[]}",
        // an impossible date in the log
        "{\"v\":1,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[[20261232,1,1,0]]}",
        // log out of order
        "{\"v\":1,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],"
        "\"log\":[[20261216,1,1,0],[20261215,1,1,0]]}",
        // a negative
        "{\"v\":1,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[[20261216,1,1,-1]]}",
    };
    for (const char* t : bad) {
        PlanState s;
        EXPECT_FALSE(s.fromJson(t, std::strlen(t))) << t;
        EXPECT_FALSE(s.active());
    }
    PlanState s;
    EXPECT_FALSE(s.fromJson(nullptr, 0));
}
