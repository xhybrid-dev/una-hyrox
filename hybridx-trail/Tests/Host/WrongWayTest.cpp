/**
 * @file    WrongWayTest.cpp
 * @brief   Trail::WrongWay: on the route, running it backwards (T4, Jon's
 *          first field test). When the watch says so, and when it must not.
 *
 * The chain tests run the real RouteTracker, OffCourse and WrongWay together,
 * one fix a second, as Navigator::update does: off course wins a fix that
 * raises both.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "OffCourse.hpp"
#include "RouteTracker.hpp"
#include "WrongWay.hpp"
#include "support/RunSim.hpp"

using RunSim::NE;
using Trail::GeoPoint;
using Trail::OffCourse;
using Trail::RouteTracker;
using Trail::WrongWay;
using Event = OffCourse::Event;

namespace
{

/// The tracker and both alerts on one route.
struct Chain {
    std::vector<GeoPoint>    pts;
    std::vector<float>       cum;
    RouteTracker             tracker;
    OffCourse                off;
    WrongWay                 wrong;
    uint32_t                 ms = 0;
    std::vector<std::string> events;   ///< every alert, in order
    std::vector<uint32_t>    at;       ///< and the second it came

    explicit Chain(const std::vector<NE>& way, double spacingM = 25.0)
        : pts(RunSim::route(way, spacingM))
        , cum(pts.size())
        , tracker(pts.data(), static_cast<uint16_t>(pts.size()), cum.data())
    {
    }

    void fix(const GeoPoint& f, float precision = 4.0f)
    {
        ms += 1000u;
        const RouteTracker::Position& p = tracker.update(f);
        Event       e = off.update(ms, p, precision);
        const Event w = wrong.update(ms, p, precision, off.state() == OffCourse::State::Off);
        if (e == Event::None) {
            e = w;
        }
        if (e != Event::None) {
            events.push_back(OffCourse::name(e));
            at.push_back(ms / 1000u);
        }
    }

    void run(const std::vector<GeoPoint>& fixes)
    {
        for (const GeoPoint& f : fixes) {
            fix(f);
        }
    }

    int count(const char* name) const
    {
        int n = 0;
        for (const auto& e : events) {
            n += e == name ? 1 : 0;
        }
        return n;
    }
};

RouteTracker::Position at(float alongM, float offM = 3.0f)
{
    RouteTracker::Position p;
    p.everLocked = true;
    p.onRoute    = true;
    p.alongM     = alongM;
    p.offRouteM  = offM;
    return p;
}

const std::vector<NE> kStraight { { 0, 0 }, { 1000, 0 } };
const std::vector<NE> kSquareLoop { { 0, 0 }, { 500, 0 }, { 500, 500 }, { 0, 500 }, { 0, 0 } };
const std::vector<NE> kOutAndBack { { 0, 0 }, { 1000, 0 }, { 0, 0 } };
const std::vector<NE> kBowTie { { 0, 0 }, { 100, 100 }, { 100, 0 }, { 0, 100 }, { 0, 0 } };

std::vector<NE> reversed(std::vector<NE> way)
{
    return std::vector<NE>(way.rbegin(), way.rend());
}

} // namespace

// -- The state machine on its own ---------------------------------------------------

TEST(WrongWay, NothingBeforeTheRouteNorAfterTheFinish)
{
    WrongWay w;
    RouteTracker::Position before;   // never locked
    for (uint32_t s = 0; s < 100; ++s) {
        EXPECT_EQ(w.update(s * 1000u, before, 3.0f, false), Event::None);
    }
    EXPECT_EQ(w.state(), WrongWay::State::NotStarted);

    w.update(100000u, at(500.0f), 3.0f, false);
    RouteTracker::Position done = at(1000.0f);
    done.finished               = true;
    EXPECT_EQ(w.update(101000u, done, 3.0f, false), Event::None);
    // Walking back down the route to the car park afterwards: nothing.
    for (uint32_t s = 0; s < 200; ++s) {
        EXPECT_EQ(w.update(102000u + s * 1000u, at(1000.0f - 3.0f * s), 3.0f, false), Event::None);
    }
}

TEST(WrongWay, ThirtyMetresBackHeldThreeSeconds)
{
    WrongWay w;
    uint32_t ms = 0;
    for (int i = 0; i <= 100; ++i) {   // 300 m forwards
        ASSERT_EQ(w.update(ms += 1000u, at(3.0f * i), 3.0f, false), Event::None);
    }
    // Back at 3 m/s from 300 m: 30 m back at the 10th fix, held 3 s.
    std::vector<int> fired;
    for (int i = 1; i <= 20; ++i) {
        if (w.update(ms += 1000u, at(300.0f - 3.0f * i), 3.0f, false) == Event::WentWrong) {
            fired.push_back(i);
        }
    }
    EXPECT_EQ(fired, (std::vector<int> { 13 }));
    EXPECT_TRUE(w.wrong());
}

TEST(WrongWay, AWayBackIsNeverWrong)
{
    // The tracker says the route comes back past here the other way (an
    // out-and-back's turnaround it is about to take): never counts.
    WrongWay w;
    uint32_t ms = 0;
    w.update(ms += 1000u, at(300.0f), 3.0f, false);
    for (int i = 1; i <= 30; ++i) {
        RouteTracker::Position p = at(300.0f - 3.0f * i);
        p.wayBack                = true;
        EXPECT_EQ(w.update(ms += 1000u, p, 3.0f, false), Event::None);
    }
}

TEST(WrongWay, FixesThatDoNotCountChangeNothing)
{
    WrongWay w;
    uint32_t ms = 0;
    w.update(ms += 1000u, at(300.0f), 3.0f, false);
    // 100 m back, but the GPS says +-30 m, or the fixes are 15 m off the line.
    for (int i = 1; i <= 30; ++i) {
        EXPECT_EQ(w.update(ms += 1000u, at(300.0f - 3.0f * i), 30.0f, false), Event::None);
        EXPECT_EQ(w.update(ms += 1000u, at(300.0f - 3.0f * i, 15.0f), 3.0f, false), Event::None);
    }
    EXPECT_FALSE(w.wrong());
}

TEST(WrongWay, OffCourseEndsItQuietlyAJumpCountsFromTheNewPlace)
{
    WrongWay w;
    uint32_t ms = 0;
    w.update(ms += 1000u, at(300.0f), 3.0f, false);
    for (int i = 1; i <= 15; ++i) {
        w.update(ms += 1000u, at(300.0f - 3.0f * i), 3.0f, false);
    }
    ASSERT_TRUE(w.wrong());
    EXPECT_EQ(w.update(ms += 1000u, at(250.0f), 3.0f, true), Event::None);   // off course now
    EXPECT_FALSE(w.wrong());

    for (int i = 1; i <= 15; ++i) {
        w.update(ms += 1000u, at(250.0f - 3.0f * i), 3.0f, false);
    }
    ASSERT_TRUE(w.wrong());
    RouteTracker::Position j = at(600.0f);
    j.jumped                 = true;   // rejoined further on
    EXPECT_EQ(w.update(ms += 1000u, j, 3.0f, false), Event::None);
    EXPECT_TRUE(w.wrong());   // no new alert, and not right until running forwards
    std::vector<Event> events;
    for (int i = 1; i <= 12; ++i) {   // forwards from there
        const Event e = w.update(ms += 1000u, at(600.0f + 3.0f * i), 3.0f, false);
        if (e != Event::None) {
            events.push_back(e);
        }
    }
    EXPECT_EQ(events, (std::vector<Event> { Event::RightWay }));
    EXPECT_FALSE(w.wrong());
}

TEST(WrongWay, GpsDriftWhileStandingNeverWarns)
{
    // Standing at a feed station while the fix drifts 0.4 m/s back down the
    // path for four minutes, with 5 m of noise: 96 m in all, but never 30 m
    // within the minute the anchor lasts.
    WrongWay      w;
    RunSim::Noise noise(3u);
    uint32_t      ms = 0;
    w.update(ms += 1000u, at(500.0f), 3.0f, false);
    for (int i = 0; i < 240; ++i) {
        const float along = 500.0f - 0.4f * i + static_cast<float>(noise.next(5.0));
        EXPECT_EQ(w.update(ms += 1000u, at(along), 3.0f, false), Event::None) << "second " << i;
    }
}

// -- The whole chain on real-shaped runs ------------------------------------------------

TEST(WrongWay, TurningBackOnAPointToPointRoute)
{
    // 500 m along a 1 km route, the runner turns round for 150 m (50 s), then
    // realises and runs on to the finish. With and without GPS noise.
    for (double noiseM : { 0.0, 5.0 }) {
        for (uint32_t seed = 1; seed <= 5; ++seed) {
            Chain c(kStraight);
            c.run(RunSim::run({ { 0, 0 }, { 500, 0 }, { 350, 0 }, { 1000, 0 } }, 3.0, noiseM, seed));
            EXPECT_EQ(c.events, (std::vector<std::string> { "went wrong way", "right way", "finished" }))
                << "noise " << noiseM << " seed " << seed;
            // Warned 30-45 m after turning (the turn is at fix ~167).
            ASSERT_GE(c.at.size(), 2u);
            EXPECT_GE(c.at[0], 177u) << "noise " << noiseM << " seed " << seed;
            EXPECT_LE(c.at[0], 183u) << "noise " << noiseM << " seed " << seed;
            // Right way within ~12 s of turning back (at fix ~217): 20 m on, held 3 s.
            EXPECT_LE(c.at[1], 231u) << "noise " << noiseM << " seed " << seed;
        }
    }
}

TEST(WrongWay, ALoopStartedTheWrongWayRound)
{
    // From the start of a 2 km loop, round it the other way: warned within
    // about 30 s (90 m), reminded every minute, never off course, never finished.
    for (uint32_t seed = 1; seed <= 5; ++seed) {
        Chain c(kSquareLoop);
        c.run(RunSim::run(reversed(kSquareLoop), 3.0, 4.0, seed));
        ASSERT_FALSE(c.events.empty()) << "seed " << seed;
        EXPECT_EQ(c.events[0], "went wrong way") << "seed " << seed;
        EXPECT_LE(c.at[0], 35u) << "seed " << seed;
        EXPECT_EQ(c.count("went wrong way"), 1) << "seed " << seed;   // warned once, then reminded
        EXPECT_GE(c.count("still wrong way"), 8) << "seed " << seed;  // 11 minutes
        EXPECT_EQ(c.count("went off"), 0) << "seed " << seed;
        EXPECT_EQ(c.count("finished"), 0) << "seed " << seed;
    }
}

TEST(WrongWay, CorrectRunsNeverWarn)
{
    // Every shape the tracker knows, run the right way, with GPS noise.
    std::vector<NE> zigzag;   // switchbacks 20 m apart, 150 m long
    for (int k = 0; k <= 8; ++k) {
        zigzag.push_back({ 20.0 * k, k % 2 ? 150.0 : 0.0 });
        zigzag.push_back({ 20.0 * k, k % 2 ? 0.0 : 150.0 });
    }
    const std::vector<std::vector<NE>> shapes { kStraight, kSquareLoop, kOutAndBack, kBowTie, zigzag };
    for (size_t s = 0; s < shapes.size(); ++s) {
        for (double noiseM : { 0.0, 3.0, 6.0 }) {
            for (uint32_t seed = 1; seed <= 5; ++seed) {
                Chain c(shapes[s], s == 3 ? 10.0 : 25.0);
                c.run(RunSim::run(shapes[s], s == 3 ? 2.0 : 3.0, noiseM, seed));
                EXPECT_EQ(c.count("went wrong way"), 0) << "shape " << s << " noise " << noiseM << " seed " << seed;
            }
        }
    }
}

TEST(WrongWay, ANoisyOutAndBackNeverWarns)
{
    for (double noiseM : { 5.0, 8.0 }) {
        for (uint32_t seed = 1; seed <= 20; ++seed) {
            Chain c(kOutAndBack);
            c.run(RunSim::run(kOutAndBack, 3.0, noiseM, seed));
            EXPECT_EQ(c.count("went wrong way"), 0) << "noise " << noiseM << " seed " << seed;
            EXPECT_EQ(c.count("finished"), 1) << "noise " << noiseM << " seed " << seed;
        }
    }
}

TEST(WrongWay, TurningShortOnAnOutAndBackNeverWarns)
{
    // The cone is 10 m or 25 m before the GPX's turnaround.
    for (double shortM : { 10.0, 25.0 }) {
        for (uint32_t seed = 1; seed <= 10; ++seed) {
            Chain c(kOutAndBack);
            c.run(RunSim::run({ { 0, 0 }, { 1000 - shortM, 0 }, { 0, 0 } }, 3.0, 4.0, seed));
            EXPECT_EQ(c.count("went wrong way"), 0) << "short " << shortM << " seed " << seed;
        }
    }
}

TEST(WrongWay, StandingStillWithNoisyGpsNeverWarns)
{
    Chain c(kStraight);
    c.run(RunSim::run({ { 0, 0 }, { 300, 0 } }, 3.0));
    RunSim::Noise noise(11u);
    for (int i = 0; i < 300; ++i) {   // five minutes, 8 m of wander
        c.fix(RunSim::at({ 300 + noise.next(8.0), noise.next(8.0) }));
    }
    EXPECT_EQ(c.count("went wrong way"), 0);
}

TEST(WrongWay, ARejoinBehindAfterADetourIsNotWrong)
{
    // Off at 300 m for a 100 m detour, back onto the route 50 m behind where
    // it was left, then on to the finish: off, back on, finished. No wrong way.
    Chain c(kStraight);
    c.run(RunSim::run({ { 0, 0 }, { 300, 0 }, { 300, 60 }, { 250, 60 }, { 250, 0 }, { 1000, 0 } }, 3.0, 3.0, 4u));
    EXPECT_EQ(c.events, (std::vector<std::string> { "went off", "back on", "finished" }));
}

TEST(WrongWay, OffCourseWhileWrongTakesOver)
{
    // Wrong way, then off the route altogether: the off-course alert, and the
    // wrong way is dropped (the banner shows off course).
    Chain c(kStraight);
    c.run(RunSim::run({ { 0, 0 }, { 500, 0 }, { 420, 0 }, { 420, 100 } }, 3.0));
    EXPECT_EQ(c.events, (std::vector<std::string> { "went wrong way", "went off" }));
    EXPECT_FALSE(c.wrong.wrong());
}

TEST(WrongWay, ADeliberateReverseRunKeepsReminding)
{
    // The runner chooses to run a 2 km route from its finish to its start:
    // warned once at the start, then reminded every minute (a "reverse route"
    // option is a later feature).
    const std::vector<NE> line { { 0, 0 }, { 2000, 0 } };
    Chain                 c(line);
    c.run(RunSim::run(reversed(line), 3.0, 3.0, 9u));
    ASSERT_FALSE(c.events.empty());
    EXPECT_EQ(c.events[0], "went wrong way");
    EXPECT_GE(c.count("still wrong way"), 9);   // 11 minutes
    EXPECT_EQ(c.count("finished"), 0);
}
