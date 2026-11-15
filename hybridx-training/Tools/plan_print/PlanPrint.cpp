/**
 * @file    PlanPrint.cpp
 * @brief   A whole plan as Markdown. See PlanPrint.hpp.
 */

#include "PlanPrint.hpp"

#include "PaceModel.hpp"
#include "PlanBuilder.hpp"
#include "PlanCalendar.hpp"
#include "PlanSchedule.hpp"
#include "PlanState.hpp"
#include "PlanText.hpp"
#include "TimeText.hpp"
#include "data/PlanData.hpp"

#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstdarg>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace XTraining {

namespace {

const char* const kDay[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
const char* const kMonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

std::string fmt(const char* f, ...) __attribute__((format(printf, 1, 2)));

std::string fmt(const char* f, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof(buf), f, ap);
    va_end(ap);
    return buf;
}

std::string pace(uint32_t s)
{
    char b[16];
    TimeText::duration(s, b, sizeof(b));
    return b;
}

std::string dayDate(Ymd d)
{
    return fmt("%s %d %s", kDay[static_cast<int>(Date::weekday(d))], Date::day(d), kMonth[Date::month(d) - 1]);
}

std::string hm(uint32_t sec)
{
    const uint32_t m = (sec + 30) / 60;
    return fmt("%u:%02u", m / 60, m % 60);
}

std::string kmRounded(uint32_t m)
{
    const uint32_t tenths = (m + 50) / 100;
    return fmt("%u.%u km", tenths / 10, tenths % 10);
}

uint32_t paceOfMmps(uint32_t mmps) { return mmps == 0 ? 0 : (1000000u + mmps / 2) / mmps; }

std::string stepText(const SDK::Workout::Step& s, AlertMode alert)
{
    using SDK::Workout::StepEnd;
    using SDK::Workout::Target;
    char d[24] = {};
    switch (s.end) {
    case StepEnd::Time:
        PlanText::repTime(s.durationMs / 1000u, d, sizeof(d));
        break;
    case StepEnd::Distance:
        PlanText::distance(s.distanceCm, d, sizeof(d));
        break;
    default:
        std::snprintf(d, sizeof(d), "until lap");
        break;
    }
    std::string t;
    if (s.target == Target::Speed) {
        const uint32_t fast = paceOfMmps(s.speedHighMmps);
        const uint32_t slow = paceOfMmps(s.speedLowMmps);
        if (alert == AlertMode::FastOnly) {
            t = " @ " + pace(fast) + "-" + pace(slow) + " (easy: too fast only)";
        } else if (alert == AlertMode::AtEnd) {
            t = " @ " + pace(fast) + "-" + pace(slow) + " (judged at the end)";
        } else if (alert == AlertMode::None) {
            t = " (fast: about " + pace(paceOfMmps((s.speedLowMmps + s.speedHighMmps) / 2u)) + ")";
        } else {
            t = " @ " + pace(fast) + "-" + pace(slow);
        }
    } else if (s.target == Target::HeartRate) {
        t = fmt(" @ zone %u%s", static_cast<unsigned>(s.hrZone),
                alert == AlertMode::FastOnly ? " (too high only)" : "");
    }
    return std::string(s.name) + " " + d + t;
}

/// The program as one line: "Warm up 15 min; 6 x [Interval 3 min @ ..., Jog 1:45]; Cool down 10 min".
std::string stepsLine(const SDK::Workout::Program& p, const SessionMeta& m)
{
    std::vector<std::pair<uint16_t, std::string>> tokens;
    for (uint16_t i = 0; i < p.stepCount; ++i) {
        const auto& s = p.steps[i];
        if (s.end == SDK::Workout::StepEnd::Repeat) {
            std::string inner;
            while (!tokens.empty() && tokens.back().first >= s.repeatFrom) {
                inner = tokens.back().second + (inner.empty() ? "" : ", " + inner);
                tokens.pop_back();
            }
            tokens.push_back({s.repeatFrom, fmt("%u x [", static_cast<unsigned>(s.repeatCount)) + inner + "]"});
        } else {
            tokens.push_back({i, stepText(s, m.alert[i])});
        }
    }
    std::string out;
    for (const auto& t : tokens) {
        out += (out.empty() ? "" : "; ") + t.second;
    }
    return out;
}

const char* phaseName(Phase p)
{
    switch (p) {
    case Phase::Base:     return "base";
    case Phase::Build:    return "build";
    case Phase::Peak:     return "peak";
    case Phase::Specific: return "specific";
    case Phase::Taper:    return "taper";
    case Phase::RaceWeek: return "race week";
    case Phase::CutBack:  return "cut-back";
    case Phase::Baseline: return "baseline";
    case Phase::Retest:   return "retest";
    }
    return "";
}

const char* statusText(Calendar::Status s)
{
    switch (s) {
    case Calendar::Status::Ok:        return "the whole plan";
    case Calendar::Status::Truncated: return "base weeks dropped to fit the race";
    case Calendar::Status::TooClose:  return "too close to the race for the whole plan: the final weeks only";
    case Calendar::Status::RaceGone:  return "the race is before the plan can start";
    case Calendar::Status::BadDate:   return "no valid date";
    }
    return "";
}

}  // namespace

std::string dayList(DayMask days)
{
    std::string s;
    for (uint8_t d = 0; d < 7; ++d) {
        if ((days >> d) & 1u) {
            s += (s.empty() ? "" : " ") + std::string(kDay[d]);
        }
    }
    return s;
}

std::string printPlan(const PrintInput& in)
{
    const PlanDef* plan = PlanData::find(in.plan);
    if (plan == nullptr) {
        return "No such plan.\n";
    }
    const PaceResult pr = PaceModel::compute({in.fiveKSec, in.halfSec});
    if (!pr.ok) {
        return "The 5K time is missing or out of range (12:00 to 59:59).\n";
    }
    if (!WeekLayout::valid(in.days, in.longDay)) {
        return "Choose 3 to 6 run days, the long-run day among them.\n";
    }
    PlanSetup setup;
    setup.plan = in.plan;
    setup.level = in.level;
    setup.days = in.days;
    setup.longDay = in.longDay;
    setup.easyByHr = in.easyByHr;
    setup.repsByDistance = in.repsByDistance;
    setup.raceDate = in.raceDate;
    setup.fiveKSec = in.fiveKSec;
    setup.fiveKRecent = in.fiveKRecent;
    setup.halfSec = in.halfSec;
    const Calendar cal = PlanCalendar::make({in.setupDay, in.raceDate, plan->weekCount, plan->minWeeks});

    std::string o;
    o += fmt("# %s plan: 5K %s, level %u, %s, long run %s\n\n", plan->name, pace(in.fiveKSec).c_str(),
             static_cast<unsigned>(in.level), dayList(in.days).c_str(), kDay[in.longDay]);
    o += "Generated by `plan_print` from the plan files (`Tools/Plans/`) with the app's own code: "
         "these are the sessions the watch would build. Times and distances marked \"about\" are "
         "estimates at the middle of each step's band.\n\n";
    std::string options;
    if (in.halfSec != 0) {
        options += "half marathon " + pace(in.halfSec) + "; ";
    }
    options += in.fiveKRecent ? "5K recent; " : "5K not recent (week 1 is a time-trial week); ";
    options += in.easyByHr ? "easy runs by heart rate; " : "";
    options += in.repsByDistance ? "reps by distance; " : "reps by time; ";
    options.resize(options.size() - 2);
    o += "Options: " + options + ".\n\n";

    const Paces& p = pr.paces;
    o += "## Paces (per km)\n\n";
    o += "| Easy | Marathon | Half | Threshold | 10K | Interval | Repetition | 5K |\n";
    o += "|---|---|---|---|---|---|---|---|\n";
    o += "| " + pace(p.easyFast) + "-" + pace(p.easySlow) + " | " + pace(p.marathon) + " | " + pace(p.half) + " | "
         + pace(p.threshold) + " | " + pace(p.tenK) + " | " + pace(p.interval) + " | " + pace(p.repetition) + " | "
         + pace(p.fiveK) + " |\n\n";
    o += "Predicted from the 5K: 10K " + pace(pr.predicted10kSec) + ", half " + pace(pr.predictedHalfSec)
         + ", marathon " + pace(pr.predictedMarathonSec) + ".";
    if (in.halfSec != 0) {
        o += " The half matches a 5K of " + pace(pr.halfAs5kSec)
             + (pr.enduranceGap ? ": endurance is behind speed, so the long runs matter most."
                                : ": no gap worth a word.");
    }
    o += "\n\n";

    o += "## Calendar\n\n";
    if (!cal.usable()) {
        o += std::string("No plan: ") + statusText(cal.status) + ".\n";
        return o;
    }
    o += fmt("Set up %s; starts %s, %u weeks (%s", dayDate(in.setupDay).c_str(), dayDate(cal.start).c_str(),
             static_cast<unsigned>(cal.weeks), statusText(cal.status));
    if (cal.firstWeek > 0) {
        o += fmt(", from the plan's week %u", static_cast<unsigned>(cal.firstWeek + 1));
    }
    o += ").";
    if (in.raceDate != 0) {
        o += " Race " + dayDate(in.raceDate) + ".";
    }
    const Scaling sc = PlanBuilder::scaling(*plan, in.level, p);
    if (sc.longPermille != 1000) {
        o += fmt(" Long runs scaled by %u.%03u to reach the plan's %u km floor.",
                 static_cast<unsigned>(sc.longPermille / 1000), static_cast<unsigned>(sc.longPermille % 1000),
                 static_cast<unsigned>(plan->longFloorM[in.level - 1] / 1000));
    }
    o += "\n\n";

    BuildOptions bo;
    bo.level = in.level;
    bo.easyByHr = in.easyByHr;
    bo.repsByDistance = in.repsByDistance;
    auto program = std::make_unique<SDK::Workout::Program>();
    SessionMeta meta;

    uint32_t planSec = 0, planM = 0, planQ = 0, runs = 0, peakM = 0;
    std::string summary = "| Week | Phase | Runs | Time | Distance | Quality |\n|---|---|---|---|---|---|\n";
    for (uint8_t w = 0; w < cal.weeks; ++w) {
        DaySession days[7];
        if (!PlanSchedule::week(setup, *plan, cal, p, w, days)) {
            o += fmt("Week %u could not be placed.\n", static_cast<unsigned>(w + 1));
            continue;
        }
        o += fmt("## Week %u (%s), from %s\n\n", static_cast<unsigned>(w + 1), phaseName(days[0].phase),
                 dayDate(days[0].date).c_str());
        o += "| Day | Session | Steps | About |\n|---|---|---|---|\n";
        uint32_t wSec = 0, wM = 0, wQ = 0, wRuns = 0;
        for (const DaySession& d : days) {
            if (d.asRun.kind == Kind::Rest) {
                continue;
            }
            if (!PlanBuilder::build(d.asRun, p, bo, plan->raceCm, *program, meta)) {
                o += "| " + dayDate(d.date) + " | (could not be built) | | |\n";
                continue;
            }
            char what[48];
            PlanText::what(d.asRun, plan->raceCm, what, sizeof(what));
            std::string name = std::string(PlanText::name(d.asRun.kind)) + ", " + what + (d.key ? " (key)" : "");
            if (meta.repM > 0 && (d.asRun.kind == Kind::Intervals || d.asRun.kind == Kind::Cruise
                                  || d.asRun.kind == Kind::Pace10k || d.asRun.kind == Kind::PaceHalf)) {
                name += fmt(", about %u m a rep", static_cast<unsigned>(meta.repM));
            }
            o += "| " + dayDate(d.date) + " | " + name + " | " + stepsLine(*program, meta) + " | "
                 + kmRounded(meta.plannedM) + ", " + hm(meta.plannedSec) + " |\n";
            wSec += meta.plannedSec;
            wM += meta.plannedM;
            wQ += meta.qualitySec;
            ++wRuns;
        }
        o += fmt("\nWeek: %u runs, %s, about %s; %u min of quality.\n\n", static_cast<unsigned>(wRuns),
                 hm(wSec).c_str(), kmRounded(wM).c_str(), static_cast<unsigned>((wQ + 30) / 60));
        summary += fmt("| %u | %s | %u | %s | %s | %u min |\n", static_cast<unsigned>(w + 1), phaseName(days[0].phase),
                       static_cast<unsigned>(wRuns), hm(wSec).c_str(), kmRounded(wM).c_str(),
                       static_cast<unsigned>((wQ + 30) / 60));
        planSec += wSec;
        planM += wM;
        planQ += wQ;
        runs += wRuns;
        peakM = wM > peakM ? wM : peakM;
    }
    o += "## The whole plan\n\n" + summary;
    o += fmt("\n%u runs, %s in all, about %s; the biggest week about %s. Quality is %u%% of the time.\n",
             static_cast<unsigned>(runs), hm(planSec).c_str(), kmRounded(planM).c_str(), kmRounded(peakM).c_str(),
             static_cast<unsigned>(planSec == 0 ? 0 : (planQ * 100u + planSec / 2) / planSec));
    return o;
}

}  // namespace XTraining
