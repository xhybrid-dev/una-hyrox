/**
 * @file    OffCourseTest.cpp
 * @brief   Trail::OffCourse: when the watch buzzes, and when it must not.
 *
 * The scenarios double as the walk-through for Gate T1 (NOTES, "T1"): each is
 * a situation a trail runner would recognise, with the buzzes it produces.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "OffCourse.hpp"
#include "RouteTracker.hpp"
#include "support/RunSim.hpp"

using RunSim::NE;
using Trail::GeoPoint;
using Trail::OffCourse;
using Trail::RouteTracker;
using Event = OffCourse::Event;
using State = OffCourse::State;

namespace
{

RouteTracker::Position at(float offM, bool locked = true, bool finished = false)
{
    RouteTracker::Position p;
    p.everLocked = locked;
    p.onRoute    = offM <= RouteTracker::kAcquireM;
    p.offRouteM  = offM;
    p.finished   = finished;
    return p;
}

/// Feed one reading a second from @p startMs; return every event, tagged with its second.
std::vector<std::string> feed(OffCourse& oc, const std::vector<float>& offM, uint32_t startMs = 0,
                              float precision = 3.0f)
{
    std::vector<std::string> events;
    for (size_t i = 0; i < offM.size(); ++i) {
        const Event e = oc.update(startMs + static_cast<uint32_t>(i) * 1000u, at(offM[i]), precision);
        if (e != Event::None) {
            events.push_back(std::to_string(i) + "s " + OffCourse::name(e));
        }
    }
    return events;
}

std::vector<float> repeat(float v, int n)
{
    return std::vector<float>(static_cast<size_t>(n), v);
}

std::vector<float> operator+(std::vector<float> a, const std::vector<float>& b)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

OffCourse started()
{
    OffCourse oc;
    oc.update(0, at(5.0f), 3.0f);   // reached the route
    EXPECT_EQ(oc.state(), State::OnCourse);
    return oc;
}

} // namespace

TEST(OffCourse, WalkingToTheStartIsNotBeingLost)
{
    OffCourse oc;
    for (uint32_t s = 0; s < 600; ++s) {   // ten minutes, 800 m from the route
        EXPECT_EQ(oc.update(s * 1000u, at(800.0f, false), 3.0f), Event::None);
    }
    EXPECT_EQ(oc.state(), State::NotStarted);
}

TEST(OffCourse, AWrongTurnBuzzesOnceThenRemindsThenClears)
{
    OffCourse oc     = started();
    // 20 s on the line, 150 s on a wrong path 70-120 m off, 10 s at 40 m
    // (between the thresholds: still off), then back on the line from 180 s.
    const auto trace = repeat(5, 20) + repeat(70, 5) + repeat(120, 145) + repeat(40, 10) + repeat(10, 10);
    EXPECT_EQ(feed(oc, trace, 1000),
              (std::vector<std::string> { "25s went off", "85s still off", "145s still off", "183s back on" }));
    EXPECT_EQ(oc.state(), State::OnCourse);
}

TEST(OffCourse, AGpsSpikeNeverBuzzes)
{
    OffCourse oc = started();
    // Four seconds of a fix jumping 90 m (under trees), between good fixes.
    EXPECT_TRUE(feed(oc, repeat(5, 10) + repeat(90, 4) + repeat(5, 10) + repeat(90, 4) + repeat(5, 10), 1000).empty());
}

TEST(OffCourse, HoveringBetweenTheThresholdsDoesNotChatter)
{
    OffCourse oc = started();
    // A switchback 35-48 m from the GPX line: never far enough to go off.
    std::vector<float> wobble;
    for (int i = 0; i < 120; ++i) {
        wobble.push_back(i % 2 ? 48.0f : 35.0f);
    }
    EXPECT_TRUE(feed(oc, wobble, 1000).empty());

    // Once off, the same wobble is never near enough to count as back.
    OffCourse off = started();
    feed(off, repeat(80, 10), 1000);
    ASSERT_EQ(off.state(), State::Off);
    OffCourse::Config quiet;
    quiet.remindMs = 0;
    OffCourse offQuiet(quiet);
    offQuiet.update(0, at(5.0f), 3.0f);
    feed(offQuiet, repeat(80, 10), 1000);
    EXPECT_TRUE(feed(offQuiet, wobble, 20000).empty());
    EXPECT_EQ(offQuiet.state(), State::Off);
}

TEST(OffCourse, BadFixesAreIgnored)
{
    OffCourse oc = started();
    // 30 s reading 80 m off, but with the GPS itself saying +-40 m.
    EXPECT_TRUE(feed(oc, repeat(80, 30), 1000, 40.0f).empty());
    EXPECT_EQ(oc.state(), State::OnCourse);
    // Precision 0 (not reported) still counts.
    EXPECT_EQ(feed(oc, repeat(80, 6), 40000, 0.0f), (std::vector<std::string> { "5s went off" }));
}

TEST(OffCourse, ABadFixRestartsTheCount)
{
    OffCourse oc = started();
    for (uint32_t s = 1; s <= 4; ++s) {
        EXPECT_EQ(oc.update(s * 1000u, at(80.0f), 3.0f), Event::None);
    }
    EXPECT_EQ(oc.update(5000u, at(80.0f), 60.0f), Event::None);   // bad fix
    for (uint32_t s = 6; s <= 10; ++s) {
        EXPECT_EQ(oc.update(s * 1000u, at(80.0f), 3.0f), Event::None);
    }
    EXPECT_EQ(oc.update(11000u, at(80.0f), 3.0f), Event::WentOff);   // 5 s after the count restarted
}

TEST(OffCourse, FinishingEndsEverything)
{
    OffCourse oc = started();
    EXPECT_EQ(oc.update(1000, at(5.0f, true, true), 3.0f), Event::Finished);
    EXPECT_EQ(oc.state(), State::Finished);
    // Walking off to the car park afterwards buzzes nothing.
    EXPECT_TRUE(feed(oc, repeat(300, 120), 2000).empty());
}

TEST(OffCourse, RightAcrossTheClockWrap)
{
    OffCourse oc;
    const uint32_t start = 0xFFFFF000u;   // about 4 s before the millisecond clock wraps
    oc.update(start, at(5.0f), 3.0f);
    EXPECT_EQ(feed(oc, repeat(80, 7), start + 1000u), (std::vector<std::string> { "5s went off" }));
    EXPECT_EQ(oc.offForMs(start + 1000u + 6000u), 6000u);
}

TEST(OffCourse, WithTheTrackerOnAWrongTurn)
{
    // The whole chain: a 1 km route, a runner who misses a turn at 300 m,
    // runs 200 m wrong, and cuts back to the route at 700 m.
    const std::vector<NE> way { { 0, 0 }, { 1000, 0 } };
    const std::vector<NE> ran { { 0, 0 }, { 300, 0 }, { 400, 200 }, { 600, 200 }, { 700, 0 }, { 1000, 0 } };
    auto                  pts = RunSim::route(way);
    std::vector<float>    cum(pts.size());
    RouteTracker          tracker(pts.data(), static_cast<uint16_t>(pts.size()), cum.data());
    OffCourse             oc;

    std::vector<std::string> events;
    uint32_t                 ms = 0;
    for (const GeoPoint& f : RunSim::run(ran, 3.0, 4.0)) {
        const Event e = oc.update(ms, tracker.update(f), 4.0f);
        if (e != Event::None) {
            events.push_back(OffCourse::name(e));
        }
        ms += 1000u;
    }
    // The detour is ~650 m, over 3.5 minutes at 3 m/s: three reminders.
    EXPECT_EQ(events, (std::vector<std::string> { "went off", "still off", "still off", "still off", "back on",
                                                  "finished" }));
}
