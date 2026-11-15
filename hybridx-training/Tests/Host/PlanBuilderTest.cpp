#include "PlanBuilder.hpp"
#include "PlanText.hpp"
#include "WeekLayout.hpp"
#include "data/PlanData.hpp"

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWorkoutReader.hpp"
#include "SDK/Fit/FitWorkoutWriter.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"

#include "FakeFileSystem.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

using namespace XTraining;
using SDK::Workout::Program;
using SDK::Workout::Step;
using SDK::Workout::StepEnd;
using SDK::Workout::Target;

namespace {

Paces pacesFor(uint32_t fiveK, uint32_t half = 0) { return PaceModel::compute({fiveK, half}).paces; }

/// The reader's rules for a valid program (FitWorkoutReader.hpp, "What it checks").
::testing::AssertionResult validProgram(const Program& p)
{
    if (p.stepCount == 0 || p.stepCount > SDK::Workout::kMaxSteps) {
        return ::testing::AssertionFailure() << "step count " << p.stepCount;
    }
    // Open blocks as [from, end] pairs, to check nesting.
    std::vector<std::pair<int, int>> blocks;
    for (uint16_t i = 0; i < p.stepCount; ++i) {
        const Step& s = p.steps[i];
        if (s.degraded) {
            return ::testing::AssertionFailure() << "step " << i << " degraded";
        }
        if (s.end == StepEnd::Repeat) {
            if (s.repeatFrom >= i) {
                return ::testing::AssertionFailure() << "repeat " << i << " does not point back";
            }
            if (s.repeatCount < 1 || s.repeatCount > SDK::Workout::kMaxRepeatCount) {
                return ::testing::AssertionFailure() << "repeat count " << s.repeatCount;
            }
            for (const auto& b : blocks) {
                const bool inside = b.first >= s.repeatFrom && b.second < i;
                const bool apart = b.second < s.repeatFrom;
                if (!inside && !apart) {
                    return ::testing::AssertionFailure() << "blocks overlap at " << i;
                }
            }
            blocks.push_back({s.repeatFrom, i});
            int depth = 0;
            for (const auto& b : blocks) {
                depth += b.first >= s.repeatFrom && b.second <= i;
            }
            if (depth > SDK::Workout::kMaxRepeatDepth) {
                return ::testing::AssertionFailure() << "nesting " << depth;
            }
        } else {
            if (s.end == StepEnd::Time && s.durationMs == 0) {
                return ::testing::AssertionFailure() << "step " << i << ": zero time";
            }
            if (s.end == StepEnd::Distance && s.distanceCm == 0) {
                return ::testing::AssertionFailure() << "step " << i << ": zero distance";
            }
            if (s.target == Target::Speed && !(s.speedLowMmps > 0 && s.speedLowMmps < s.speedHighMmps)) {
                return ::testing::AssertionFailure() << "step " << i << ": bad band";
            }
            if (s.name[0] == '\0') {
                return ::testing::AssertionFailure() << "step " << i << ": no name";
            }
        }
    }
    if (p.steps[p.stepCount - 1].end == StepEnd::Repeat && p.stepCount == 1) {
        return ::testing::AssertionFailure() << "only a repeat";
    }
    return ::testing::AssertionSuccess();
}

/// Runs @p p through the SDK's engine, a runner on the band's middle (or easy
/// pace), pressing lap after 60 s on an open step. Returns the steps entered
/// and sets @p seconds and @p finished.
uint64_t simulate(const Program& p, const Paces& paces, uint32_t& seconds, bool& finished, bool& reachedEnd)
{
    SDK::Workout::Engine e;
    e.start(p, 0, 0);
    uint32_t tMs = 0, dCm = 0;
    uint64_t entered = 1;
    const uint32_t easyMmps = (PaceModel::mmps(paces.easyFast) + PaceModel::mmps(paces.easySlow)) / 2;
    // Recoveries with no target are jogged, a little slower than easy, as the
    // builder's estimates assume.
    const uint32_t jogMmps = PaceModel::mmps(paces.easySlow + 45u);
    for (int guard = 0; guard < 8 * 3600 && !e.status().finished; ++guard) {
        const Step& s = p.steps[e.status().step];
        if (s.end == StepEnd::Open && e.status().stepTimeMs >= 60000u) {
            if (e.next(tMs, dCm) != SDK::Workout::Engine::Change::None) {
                ++entered;
            }
            continue;
        }
        const uint32_t v = s.target == Target::Speed ? (s.speedLowMmps + s.speedHighMmps) / 2
                         : s.intensity == SDK::Workout::Intensity::Rest ? jogMmps : easyMmps;
        tMs += 1000;
        dCm += v / 10;
        if (e.update(tMs, dCm, v, true) != SDK::Workout::Engine::Change::None) {
            ++entered;
        }
    }
    seconds = tMs / 1000;
    finished = e.status().finished;
    reachedEnd = e.status().reachedEnd;
    return finished ? entered - 1 : entered;  // the last change ends the workout, not a new step
}

bool hasOpenStep(const Program& p)
{
    for (uint16_t i = 0; i < p.stepCount; ++i) {
        if (p.steps[i].end == StepEnd::Open) {
            return true;
        }
    }
    return false;
}

// FIT round trip, as the SDK's own FitWorkoutWriter_test.cpp does it.
SDK::Fit::FitWorkoutReader::Result roundTrip(const Program& program, Program& back)
{
    namespace fit = SDK::Fit;
    constexpr fit::WorkoutLocalTypes kLocal{1, 2, 3};
    SDK::Test::FakeFileSystem fs;
    {
        auto file = fs.file("w.fit");
        EXPECT_TRUE(file->open(true, true));
        fit::FitWriter w(*file);
        EXPECT_TRUE(w.begin(0));
        EXPECT_TRUE(w.defineMessage(0, fit::mesgNum(fit::MesgNum::FileId), {fit::field::FileId::Type}));
        EXPECT_TRUE(w.data(0).u8(static_cast<uint8_t>(fit::File::Workout)).write());
        EXPECT_TRUE(fit::writeWorkout(w, program, kLocal));
        EXPECT_TRUE(w.finish());
        file->close();
    }
    fit::FitWorkoutReader reader;
    auto file = fs.file("w.fit");
    return reader.read(*file, back);
}

::testing::AssertionResult sameProgram(const Program& a, const Program& b)
{
    if (a.stepCount != b.stepCount) {
        return ::testing::AssertionFailure() << "steps " << a.stepCount << " vs " << b.stepCount;
    }
    if (std::strcmp(a.name, b.name) != 0 || a.sport != b.sport || a.subSport != b.subSport) {
        return ::testing::AssertionFailure() << "name or sport";
    }
    for (uint16_t i = 0; i < a.stepCount; ++i) {
        const Step& x = a.steps[i];
        const Step& y = b.steps[i];
        const bool same = x.end == y.end && x.durationMs == y.durationMs && x.distanceCm == y.distanceCm
                       && x.repeatFrom == y.repeatFrom && x.repeatCount == y.repeatCount
                       && x.target == y.target && x.speedLowMmps == y.speedLowMmps
                       && x.speedHighMmps == y.speedHighMmps && x.hrKind == y.hrKind && x.hrZone == y.hrZone
                       && x.intensity == y.intensity && std::strcmp(x.name, y.name) == 0 && !y.degraded;
        if (!same) {
            return ::testing::AssertionFailure() << "step " << i << " (" << x.name << ") differs";
        }
    }
    return ::testing::AssertionSuccess();
}

struct Each {
    const PlanDef* plan;
    const Session* session;
    uint8_t week;  // 99 for the trial or recent week
};

std::vector<Each> everySession()
{
    std::vector<Each> out;
    for (const PlanDef& p : PlanData::kPlans) {
        for (uint8_t w = 0; w < p.weekCount; ++w) {
            for (const Session& s : p.weeks[w].slot) {
                out.push_back({&p, &s, w});
            }
        }
        for (const Week* extra : {p.trialWeek, p.recentWeek}) {
            if (extra != nullptr) {
                for (const Session& s : extra->slot) {
                    out.push_back({&p, &s, 99});
                }
            }
        }
    }
    return out;
}

/// The capped quantity of a session, in seconds or metres, and its cap.
void checkCaps(const Session& s, uint8_t level)
{
    const LevelCaps& c = PlanBuilder::caps(level);
    const uint32_t total = static_cast<uint32_t>(s.reps) * s.a;
    switch (s.kind) {
    case Kind::Intervals:
        EXPECT_LE(total, c.intervalSec);
        EXPECT_LE(s.a, c.intervalRepMaxSec);
        EXPECT_GE(s.reps, 3);
        break;
    case Kind::Cruise:
        EXPECT_LE(total, c.cruiseSec);
        break;
    case Kind::Pace10k:
        EXPECT_LE(total, c.pace10kSec);
        break;
    case Kind::PaceHalf:
        EXPECT_LE(total, c.paceHalfSec);
        break;
    case Kind::Tempo:
        EXPECT_LE(s.a, c.tempoMin);
        break;
    case Kind::ThirtyThirty:
        EXPECT_LE(static_cast<uint32_t>(s.reps) * s.a * 30u, c.intervalSec);
        break;
    case Kind::Reps:
        ASSERT_GT(c.repsM, 0) << "no repetitions at level " << int(level);
        EXPECT_LE(total, c.repsM);
        break;
    case Kind::Long:
    case Kind::LongFinish:
        EXPECT_LE(s.a, c.longMaxMin);
        EXPECT_GE(s.a, 20);
        if (s.kind == Kind::LongFinish) {
            EXPECT_LE(s.b, c.finishMaxMin);
            EXPECT_GE(s.a, s.b + 20);
        }
        break;
    default:
        break;
    }
}

}  // namespace

TEST(PlanBuilder, EverySessionEveryLevelEveryRunner)
{
    // 5K 15:00 to 45:00, each level, with and without the two options: the
    // program is valid, the caps hold, and the SDK's engine runs it to the end.
    const std::vector<Each> all = everySession();
    auto program = std::make_unique<Program>();
    SessionMeta meta;
    int built = 0;
    for (uint32_t fiveK = 15 * 60; fiveK <= 45 * 60; fiveK += 60) {
        const Paces paces = pacesFor(fiveK);
        for (uint8_t level = 1; level <= 3; ++level) {
            for (int opt = 0; opt < 4; ++opt) {
                BuildOptions o;
                o.level = level;
                o.easyByHr = opt & 1;
                o.repsByDistance = opt & 2;
                for (const Each& e : all) {
                    SCOPED_TRACE(testing::Message() << e.plan->name << " week " << int(e.week) + 1 << " "
                                                    << PlanText::name(e.session->kind) << " 5K " << fiveK
                                                    << " level " << int(level) << " opt " << opt);
                    const Scaling sc = PlanBuilder::scaling(*e.plan, level, paces);
                    const Session s = PlanBuilder::forLevel(*e.session, sc);
                    checkCaps(s, level);
                    ASSERT_TRUE(PlanBuilder::build(s, paces, o, e.plan->raceCm, *program, meta));
                    ASSERT_TRUE(validProgram(*program));
                    ASSERT_GT(meta.plannedSec, 0u);
                    ASSERT_GT(meta.plannedM, 0u);
                    uint32_t secs = 0;
                    bool finished = false, reached = false;
                    const uint64_t entered = simulate(*program, paces, secs, finished, reached);
                    ASSERT_TRUE(finished);
                    ASSERT_TRUE(reached);
                    ASSERT_EQ(entered, SDK::Workout::totals(*program).stepsRun);
                    if (!hasOpenStep(*program)) {
                        // The estimate matches what the engine ran, within 2%.
                        const double ratio = static_cast<double>(secs) / meta.plannedSec;
                        ASSERT_NEAR(ratio, 1.0, 0.02) << secs << " vs " << meta.plannedSec;
                    }
                    ++built;
                }
            }
        }
    }
    std::printf("[ builder ] %d programs built, checked and run\n", built);
}

TEST(PlanBuilder, FitRoundTrip)
{
    const std::vector<Each> all = everySession();
    auto program = std::make_unique<Program>();
    auto back = std::make_unique<Program>();
    SessionMeta meta;
    for (uint32_t fiveK : {17u * 60, 25u * 60, 38u * 60}) {
        const Paces paces = pacesFor(fiveK);
        for (uint8_t level = 1; level <= 3; ++level) {
            for (int opt = 0; opt < 4; ++opt) {
                BuildOptions o;
                o.level = level;
                o.easyByHr = opt & 1;
                o.repsByDistance = opt & 2;
                for (const Each& e : all) {
                    SCOPED_TRACE(testing::Message() << e.plan->name << " " << PlanText::name(e.session->kind));
                    const Session s = PlanBuilder::forLevel(*e.session, PlanBuilder::scaling(*e.plan, level, paces));
                    ASSERT_TRUE(PlanBuilder::build(s, paces, o, e.plan->raceCm, *program, meta));
                    ASSERT_EQ(roundTrip(*program, *back), SDK::Fit::FitWorkoutReader::Result::Ok);
                    ASSERT_TRUE(sameProgram(*program, *back));
                }
            }
        }
    }
}

TEST(PlanBuilder, IntervalsForTheExampleRunner)
{
    // Half week 7, Tuesday: INT 5x3 for a 22:00 runner at level 2 (PLANS.md 8).
    const Paces paces = pacesFor(22 * 60);
    const PlanDef* half = PlanData::find(PlanId::Half);
    const Session s = PlanBuilder::forLevel(half->weeks[6].slot[kSlotQ1], PlanBuilder::scaling(*half, 2, paces));
    EXPECT_EQ(s.reps, 5);
    EXPECT_EQ(s.a, 180);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    ASSERT_TRUE(PlanBuilder::build(s, paces, BuildOptions{}, half->raceCm, *p, m));
    EXPECT_STREQ(p->name, "Intervals 5 x 3 min");
    EXPECT_STREQ(p->description, "Builds your VO2max");
    ASSERT_EQ(p->stepCount, 5);  // warm-up, interval, jog, repeat, cool-down
    EXPECT_EQ(p->steps[0].intensity, SDK::Workout::Intensity::Warmup);
    EXPECT_EQ(p->steps[0].durationMs, 15u * 60000u);
    EXPECT_EQ(m.alert[0], AlertMode::FastOnly);
    const Step& rep = p->steps[1];
    EXPECT_EQ(rep.durationMs, 180000u);
    EXPECT_EQ(rep.speedLowMmps, PaceModel::mmps(4 * 60 + 24));
    EXPECT_EQ(rep.speedHighMmps, PaceModel::mmps(4 * 60 + 12));
    EXPECT_EQ(m.alert[1], AlertMode::Both);
    EXPECT_EQ(p->steps[2].durationMs, 135000u);  // three quarters of 3 min, to 15 s
    EXPECT_EQ(p->steps[2].intensity, SDK::Workout::Intensity::Rest);
    EXPECT_EQ(p->steps[3].end, StepEnd::Repeat);
    EXPECT_EQ(p->steps[3].repeatCount, 5u);
    EXPECT_EQ(p->steps[4].intensity, SDK::Workout::Intensity::Cooldown);
    EXPECT_EQ(m.qualitySec, 15u * 60u);
    EXPECT_EQ(m.repM, 700u);  // 3 min at 4:18 /km: about 700 m
    EXPECT_EQ(m.targetPace, 4 * 60 + 18);
    EXPECT_EQ(m.targetPlusMinus, 6);
    // 15 + 5 x 3 + 5 x 2:15 + 10 minutes.
    EXPECT_EQ(m.plannedSec, 15u * 60 + 15 * 60 + 5 * 135 + 10 * 60);
}

TEST(PlanBuilder, Levels)
{
    const Paces paces = pacesFor(25 * 60);
    const PlanDef* tenK = PlanData::find(PlanId::TenK);
    const PlanDef* half = PlanData::find(PlanId::Half);
    const auto at = [&](const PlanDef* p, uint8_t level, Session s) {
        return PlanBuilder::forLevel(s, PlanBuilder::scaling(*p, level, paces));
    };
    // Intervals: level 1 reps to 3 min, within its 10 minutes; level 3 one more rep.
    Session s = at(tenK, 1, Session{Kind::Intervals, 4, 240, 0, 0});
    EXPECT_EQ(s.reps, 3);
    EXPECT_EQ(s.a, 180);
    s = at(tenK, 3, Session{Kind::Intervals, 4, 240, 0, 0});
    EXPECT_EQ(s.reps, 5);
    EXPECT_EQ(s.a, 240);
    // Cruise at level 3 is capped at 30 minutes (Daniels' maximum in cruise intervals).
    s = at(half, 3, Session{Kind::Cruise, 2, 900, 0, 0});
    EXPECT_EQ(s.reps, 2);
    EXPECT_EQ(s.a, 900);
    // Half pace at level 1: 25 minutes in two reps.
    s = at(half, 1, Session{Kind::PaceHalf, 2, 1200, 0, 180});
    EXPECT_EQ(s.reps, 2);
    EXPECT_EQ(s.a, 750);
    EXPECT_EQ(s.jogS, 180);
    // Repetitions are hills at level 1, two more reps at level 3.
    s = at(tenK, 1, Session{Kind::Reps, 6, 400, 0, 0});
    EXPECT_EQ(s.kind, Kind::Hills);
    EXPECT_EQ(s.reps, 4);
    s = at(tenK, 3, Session{Kind::Reps, 6, 400, 0, 0});
    EXPECT_EQ(s.reps, 8);
    // Easy runs and long runs.
    EXPECT_EQ(at(tenK, 1, Session{Kind::Easy, 0, 40, 0, 0}).a, 30);
    EXPECT_EQ(at(tenK, 3, Session{Kind::Easy, 0, 40, 0, 0}).a, 50);
    EXPECT_EQ(at(tenK, 2, Session{Kind::Long, 0, 85, 0, 0}).a, 85);
    EXPECT_EQ(at(tenK, 1, Session{Kind::Long, 0, 85, 0, 0}).a, 70);
    EXPECT_EQ(at(tenK, 3, Session{Kind::Long, 0, 85, 0, 0}).a, 100);
    EXPECT_EQ(at(tenK, 1, Session{Kind::Long, 0, 60, 0, 0}).a, 45);
    // A fast finish: half at level 1, a third more at level 3, within the caps.
    s = at(half, 1, Session{Kind::LongFinish, 0, 100, 30, 0});
    EXPECT_EQ(s.b, 15);
    s = at(half, 3, Session{Kind::LongFinish, 0, 100, 30, 0});
    EXPECT_EQ(s.b, 40);
}

TEST(PlanBuilder, TheHalfsDistanceFloor)
{
    // A 32:00 runner at level 1: 105 min at their middle easy pace is under
    // 16 km, so every long run is scaled up (PLANS.md 4.1, C9).
    const Paces slow = pacesFor(32 * 60);
    const PlanDef* half = PlanData::find(PlanId::Half);
    const Scaling sc = PlanBuilder::scaling(*half, 1, slow);
    EXPECT_GT(sc.longPermille, 1000);
    const Session peak = PlanBuilder::forLevel(Session{Kind::Long, 0, 115, 0, 0}, sc);
    const uint32_t mean = (slow.easyFast + slow.easySlow) / 2u;
    EXPECT_GE(peak.a * 60u * 1000u / mean + 300u, 16000u) << peak.a;  // within 5 min of the floor
    EXPECT_LE(peak.a, 150);
    // A 20:00 runner is already over it: no scaling.
    EXPECT_EQ(PlanBuilder::scaling(*half, 2, pacesFor(20 * 60)).longPermille, 1000);
    // The 10K plan has no floor.
    EXPECT_EQ(PlanBuilder::scaling(*PlanData::find(PlanId::TenK), 1, slow).longPermille, 1000);
}

TEST(PlanBuilder, EasyByHeartRateAndRepsByDistance)
{
    const Paces paces = pacesFor(22 * 60);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    BuildOptions o;
    o.easyByHr = true;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Long, 0, 90, 0, 0}, paces, o, 0, *p, m));
    ASSERT_EQ(p->stepCount, 1);
    EXPECT_EQ(p->steps[0].target, Target::HeartRate);
    EXPECT_EQ(p->steps[0].hrKind, SDK::Workout::HeartRateTarget::Zone);
    EXPECT_EQ(p->steps[0].hrZone, 2);
    EXPECT_EQ(p->steps[0].raw.targetValue, 2u);
    EXPECT_EQ(m.alert[0], AlertMode::FastOnly);

    o = BuildOptions{};
    o.repsByDistance = true;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Intervals, 4, 240, 0, 0}, paces, o, 0, *p, m));
    EXPECT_EQ(p->steps[1].end, StepEnd::Distance);
    EXPECT_EQ(p->steps[1].distanceCm, 90000u);  // 4 min at 4:18: 930 m, to 900 m
    EXPECT_EQ(p->steps[2].end, StepEnd::Time);  // the jog stays timed
}

TEST(PlanBuilder, ShortRepsAreJudgedAfterwards)
{
    const Paces paces = pacesFor(22 * 60);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Reps, 6, 400, 0, 0}, paces, BuildOptions{}, 0, *p, m));
    EXPECT_EQ(p->steps[1].end, StepEnd::Distance);
    EXPECT_EQ(p->steps[1].distanceCm, 40000u);
    EXPECT_EQ(m.alert[1], AlertMode::AtEnd);
    EXPECT_EQ(m.alert[2], AlertMode::None);
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::EasyStrides, 0, 40, 0, 0}, paces, BuildOptions{}, 0, *p, m));
    EXPECT_EQ(m.alert[0], AlertMode::FastOnly);
    EXPECT_EQ(m.alert[1], AlertMode::None);  // the stride
    EXPECT_EQ(p->steps[3].repeatCount, 6u);
}

TEST(PlanBuilder, ThirtyThirtyNestsTheSets)
{
    const Paces paces = pacesFor(22 * 60);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::ThirtyThirty, 3, 8, 0, 0}, paces, BuildOptions{}, 0, *p, m));
    // Warm-up, fast, easy, repeat x8, between, repeat x3, cool-down.
    ASSERT_EQ(p->stepCount, 7);
    EXPECT_EQ(p->steps[3].repeatCount, 8u);
    EXPECT_EQ(p->steps[5].repeatFrom, 1);
    EXPECT_EQ(p->steps[5].repeatCount, 3u);
    EXPECT_EQ(m.qualitySec, 3u * 8u * 30u);
    EXPECT_EQ(SDK::Workout::totals(*p).stepsRun, 1u + 3u * (8u * 2u + 1u) + 1u);
}

TEST(PlanBuilder, TimeTrialsAndTheRace)
{
    const Paces paces = pacesFor(22 * 60);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::TimeTrial5k}, paces, BuildOptions{}, 0, *p, m));
    ASSERT_EQ(p->stepCount, 3);
    EXPECT_EQ(p->steps[0].end, StepEnd::Open);
    EXPECT_EQ(p->steps[1].end, StepEnd::Distance);
    EXPECT_EQ(p->steps[1].distanceCm, 500000u);
    EXPECT_EQ(p->steps[1].target, Target::Open);
    EXPECT_EQ(p->steps[2].end, StepEnd::Open);

    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Race}, paces, BuildOptions{}, 2109750u, *p, m));
    EXPECT_STREQ(p->name, "Race 21.1 km");
    ASSERT_EQ(p->stepCount, 2);
    EXPECT_EQ(p->steps[1].distanceCm, 2109750u);
    EXPECT_EQ(p->steps[1].speedHighMmps, PaceModel::mmps(paces.half - 5));
    // A race without a race distance, and a rest day, build nothing.
    EXPECT_FALSE(PlanBuilder::build(Session{Kind::Race}, paces, BuildOptions{}, 0, *p, m));
    EXPECT_FALSE(PlanBuilder::build(Session{Kind::Rest}, paces, BuildOptions{}, 0, *p, m));
    EXPECT_EQ(p->stepCount, 0);
    // No paces, nothing built.
    EXPECT_FALSE(PlanBuilder::build(Session{Kind::Easy, 0, 30, 0, 0}, Paces{}, BuildOptions{}, 0, *p, m));
}

TEST(PlanBuilder, DefaultJogs)
{
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Cruise, 360), 60);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Cruise, 480), 90);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Cruise, 600), 120);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Cruise, 900), 180);
    // Intervals: three quarters of the rep (Daniels: 1:1; 3 min with 2 min jog, 4 with 3).
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Intervals, 120), 90);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Intervals, 180), 135);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Intervals, 240), 180);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::Intervals, 300), 225);
    EXPECT_EQ(PlanBuilder::defaultJogSec(Kind::PaceHalf, 600), 120);
}

TEST(PlanBuilder, ATempoRunIsAlwaysTimed)
{
    // Coach review: with "reps by distance" on, a tempo run stays a time. Run slowly, a
    // distance would make it longer rather than easier.
    const Paces paces = pacesFor(22 * 60);
    auto p = std::make_unique<Program>();
    SessionMeta m;
    BuildOptions o;
    o.repsByDistance = true;
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Tempo, 0, 20, 0, 0}, paces, o, 0, *p, m));
    ASSERT_EQ(p->stepCount, 3);
    EXPECT_EQ(p->steps[1].end, StepEnd::Time);
    EXPECT_EQ(p->steps[1].durationMs, 20u * 60000u);
    // The rep sessions still go by distance when asked.
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Cruise, 3, 600, 0, 0}, paces, o, 0, *p, m));
    EXPECT_EQ(p->steps[1].end, StepEnd::Distance);
    // 30-30s and hills are timed whatever the option says.
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::ThirtyThirty, 2, 8, 0, 0}, paces, o, 0, *p, m));
    EXPECT_EQ(p->steps[1].end, StepEnd::Time);
    ASSERT_TRUE(PlanBuilder::build(Session{Kind::Hills, 6, 45, 0, 0}, paces, o, 0, *p, m));
    EXPECT_EQ(p->steps[1].end, StepEnd::Time);
}

TEST(PlanBuilder, ThreeRunDaysCapTheFastFinish)
{
    // Coach review: with three run days the long run is one of only three runs, so its fast
    // finish is at most 20 minutes (level 1 is already under it).
    const Paces paces = pacesFor(25 * 60);
    const PlanDef* half = PlanData::find(PlanId::Half);
    const Session finish{Kind::LongFinish, 0, 100, 30, 0};   // the half plan's week 10
    for (uint8_t level = 1; level <= 3; ++level) {
        SCOPED_TRACE(int(level));
        const Session four = PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, level, paces, 4));
        const Session three = PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, level, paces, 3));
        EXPECT_LE(three.b, kThreeDayFinishMaxMin);
        EXPECT_LE(three.b, four.b);
        EXPECT_EQ(three.a, four.a) << "the run itself does not change";
    }
    EXPECT_EQ(PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, 2, paces, 4)).b, 30);
    EXPECT_EQ(PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, 2, paces, 3)).b, 20);
    EXPECT_EQ(PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, 3, paces, 3)).b, 20);
    EXPECT_EQ(PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, 1, paces, 3)).b, 15);
    // Five or six run days change nothing.
    EXPECT_EQ(PlanBuilder::forLevel(finish, PlanBuilder::scaling(*half, 2, paces, 5)).b, 30);
}

TEST(PlanBuilder, RaceAlertsFollowTheSetting)
{
    // Off: none; first 2 km: too fast only, then none; both: as planned.
    EXPECT_EQ(raceAlertMode(0, 0, AlertMode::Both), AlertMode::None);
    EXPECT_EQ(raceAlertMode(1, 0, AlertMode::Both), AlertMode::FastOnly);
    EXPECT_EQ(raceAlertMode(1, 199999, AlertMode::Both), AlertMode::FastOnly);
    EXPECT_EQ(raceAlertMode(1, 200000, AlertMode::Both), AlertMode::None);
    EXPECT_EQ(raceAlertMode(2, 500000, AlertMode::Both), AlertMode::Both);
    // The plan's own race steps alert both ways, which "Both" keeps.
    const Paces p = PaceModel::compute({22 * 60, 0}).paces;
    SDK::Workout::Program prog;
    SessionMeta meta;
    BuildOptions o;
    Session race;
    race.kind = Kind::Race;
    ASSERT_TRUE(PlanBuilder::build(race, p, o, 1000000u, prog, meta));
    bool found = false;
    for (uint16_t i = 0; i < prog.stepCount; ++i) {
        if (prog.steps[i].intensity == SDK::Workout::Intensity::Active) {
            EXPECT_EQ(meta.alert[i], AlertMode::Both);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}
