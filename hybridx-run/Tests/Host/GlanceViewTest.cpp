/**
 * Host tests for the VO2max glance's view: it reads exactly what
 * Vo2History::toSharedJson writes, and anything else (old version, bad
 * values, too many points, no file) reads as no data.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "GlanceView.hpp"
#include "Vo2History.hpp"

using namespace RunVo2;
using RunVo2::Glance::kGlanceRuns;
using RunVo2::Glance::makeView;
using RunVo2::Glance::View;

namespace
{
bool viewOf(const std::string& s, View& v)
{
    return makeView(s.c_str(), s.size(), v);
}
}  // namespace

TEST(GlanceView, ReadsWhatTheAppWrites)
{
    Vo2History h;
    for (uint16_t i = 0; i < 13; ++i) {
        h.add(RunRecord{1791000000u + i, static_cast<uint16_t>(480 + 5 * i), 20});
    }
    char buf[Vo2History::kMaxSharedJsonBytes];
    const size_t len = h.toSharedJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    View v;
    ASSERT_TRUE(makeView(buf, len, v));
    EXPECT_EQ(v.vo2x10, h.rollingX10());
    EXPECT_EQ(v.beforeX10, h.rollingX10Before());
    EXPECT_EQ(v.runs, kGlanceRuns);
    ASSERT_EQ(v.count, kGlanceRuns);
    uint16_t t[kGlanceRuns];
    ASSERT_EQ(h.trendX10(t, kGlanceRuns), kGlanceRuns);
    for (uint8_t i = 0; i < kGlanceRuns; ++i) {
        EXPECT_EQ(v.trend[i], t[i]) << int(i);
    }
}

TEST(GlanceView, OneRun)
{
    View v;
    ASSERT_TRUE(viewOf("{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}", v));
    EXPECT_EQ(v.vo2x10, 523);
    EXPECT_EQ(v.beforeX10, 0);
    EXPECT_EQ(v.count, 1);
}

TEST(GlanceView, NoRunsIsNoData)
{
    View v;
    EXPECT_FALSE(viewOf("{\"v\":2,\"vo2maxX10\":0,\"runs\":0,\"utc\":0,\"beforeX10\":0,\"trend\":[]}", v));
    EXPECT_FALSE(v.hasData);
}

TEST(GlanceView, AnythingElseIsNoData)
{
    const char* bad[] = {
        "",
        "not json",
        "{\"v\":1,\"vo2maxX10\":523,\"runs\":5,\"utc\":1791000000}",   // R2-R5: no trend
        "{\"v\":3,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}",
        "{\"v\":2,\"vo2maxX10\":5230,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}",
        "{\"v\":2,\"vo2maxX10\":-1,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":11,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":20,\"trend\":[523]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523,\"x\"]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[100]}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0}",
        "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,"
        "\"trend\":[500,500,500,500,500,500,500,500,500,500,500]}",
    };
    for (const char* s : bad) {
        View v;
        v.hasData = true;
        EXPECT_FALSE(viewOf(s, v)) << s;
        EXPECT_FALSE(v.hasData) << s;
        EXPECT_EQ(v.count, 0) << s;
    }
    View v;
    EXPECT_FALSE(makeView(nullptr, 10, v));
}

TEST(GlanceView, ATruncatedFileIsNoData)
{
    const std::string s = "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}";
    for (size_t n = 1; n < s.size(); ++n) {
        View v;
        EXPECT_FALSE(makeView(s.c_str(), n, v)) << n;
    }
}

TEST(GlanceView, AnOversizedFileIsNoData)
{
    std::string s = "{\"v\":2,\"vo2maxX10\":523,\"runs\":1,\"utc\":1,\"beforeX10\":0,\"trend\":[523]}";
    s.append(Glance::kMaxFileBytes, ' ');
    View v;
    EXPECT_FALSE(viewOf(s, v));
}
