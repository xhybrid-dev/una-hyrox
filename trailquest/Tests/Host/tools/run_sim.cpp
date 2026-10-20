// run_sim: whole games, start to summit, through the real game flow (Run),
// with the simulated player in Policy.hpp. It checks what balance_sim cannot:
// the XP curve (the level at each boss against the stage's par), how often a
// runner loses, the trail clock, and how long a stage takes to play.
//
//   run_sim [runs] [casual|good|menu] [--laps N] [--md out.md]
//
// --laps N plays N crossings of the Ridge (default 1): at the summit the
// runner starts the next lap (spirits five levels up, the needle sharper) and
// the report has one table a lap.
//
// The player: random route choices at forks; accepts every
// spirit's offer, letting the partner from the oldest stage go when the pack
// is full; obstacles and duels as Policy.hpp.
//
// Play time is estimated from the actions, at a pace a watch allows: a duel
// turn 5 s (choose, the beat, the result), a stretch of trail 6 s (the map,
// the choice, the walk), an obstacle 6 s, an offer to join 4 s.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Encounter.hpp"
#include "Policy.hpp"
#include "Run.hpp"

using namespace TrailQuest;

namespace
{

constexpr uint32_t kTurnSeconds     = 5;
constexpr uint32_t kTravelSeconds   = 6;
constexpr uint32_t kObstacleSeconds = 6;
constexpr uint32_t kOfferSeconds    = 4;
constexpr uint32_t kStepLimit       = 5000;   // a lap
constexpr uint32_t kMaxLaps         = 4;

struct StageStats {
    uint32_t reached     = 0;   // runs that reached its boss
    uint32_t cleared     = 0;
    uint32_t levelSum    = 0;   // level on first arrival at the boss
    uint8_t  levelMin    = 255;
    uint8_t  levelMax    = 0;
    uint32_t bossTries   = 0;
    uint32_t duelsWon    = 0;
    uint32_t duelsLost   = 0;
    uint32_t seconds     = 0;   // play time in the stage
    uint32_t clockSum    = 0;   // trail clock (minutes of the day) on clearing it
    uint32_t darkEnds    = 0;   // cleared at dusk or night
    uint32_t regularWon  = 0;
    uint32_t regularLost = 0;
    uint32_t bossFirstWins = 0; // bosses beaten at the first attempt
    uint32_t darkDuels   = 0;   // duels fought in the dark
    uint32_t windDuels   = 0;   // duels fought in the wind
    uint32_t duels       = 0;
};

uint8_t oldestPartner(const RunState& s)
{
    uint8_t slot = 0;
    for (uint8_t i = 1; i < s.partnerCount; ++i) {
        if (speciesDef(s.partners[i]).stage < speciesDef(s.partners[slot]).stage) {
            slot = i;
        }
    }
    return slot;
}

} // namespace

int main(int argc, char** argv)
{
    uint32_t    runs  = 1000;
    int         skill = 0;
    const char* md    = nullptr;
    uint32_t    laps  = 1;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--md") == 0 && i + 1 < argc) {
            md = argv[++i];
        } else if (std::strcmp(argv[i], "--laps") == 0 && i + 1 < argc) {
            laps = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            laps = laps < 1 ? 1 : (laps > kMaxLaps ? kMaxLaps : laps);
        } else if (std::strcmp(argv[i], "good") == 0) {
            skill = 1;
        } else if (std::strcmp(argv[i], "menu") == 0) {
            skill = 2;
        } else if (std::strcmp(argv[i], "casual") == 0) {
            skill = 0;
        } else {
            runs = static_cast<uint32_t>(std::strtoul(argv[i], nullptr, 10));
        }
    }
    if (runs == 0) {
        runs = 1000;
    }
    const Sim::Skill& sk = Sim::kSkills[skill];

    StageStats st[kMaxLaps][kStageCount];
    uint32_t   finished = 0, totalSeconds = 0, partnersAtEnd = 0, levelAtEnd = 0;

    for (uint32_t run_i = 0; run_i < runs; ++run_i) {
        TrailQuest::Run run;
        run.newGame(0xC0FFEEu + run_i);
        Rng      r(0xD1CEu + run_i);
        uint32_t seconds = 0;
        bool     atBossFirst[kMaxLaps][kStageCount] = {};
        uint32_t bossTriesThisStage[kMaxLaps][kStageCount] = {};

        for (uint32_t step = 0; step < kStepLimit * laps; ++step) {
            const uint8_t stage = run.state().stage;
            const uint8_t lap   = run.state().lap < kMaxLaps ? run.state().lap : kMaxLaps - 1u;
            StageStats&   ss    = st[lap][stage];
            bool done = false;
            switch (run.phase()) {
                case RunPhase::AtNode: {
                    const uint8_t exits = run.node().exitCount;
                    const ArriveResult a = run.travel(static_cast<uint8_t>(r.below(exits)));
                    seconds += kTravelSeconds;
                    ss.seconds += kTravelSeconds;
                    if (a.what == Arrival::Boss) {
                        ++ss.bossTries;
                        ++bossTriesThisStage[lap][stage];
                        if (!atBossFirst[lap][stage]) {
                            atBossFirst[lap][stage] = true;
                            ++ss.reached;
                            const uint8_t lv = run.state().level;
                            ss.levelSum += lv;
                            ss.levelMin = lv < ss.levelMin ? lv : ss.levelMin;
                            ss.levelMax = lv > ss.levelMax ? lv : ss.levelMax;
                        }
                    }
                } break;
                case RunPhase::Obstacle:
                    run.attemptObstacle(Sim::sampleBeat(Sim::scaledFor(sk, beatFor(run.state().stage, run.state().lap)), r));
                    seconds += kObstacleSeconds;
                    ss.seconds += kObstacleSeconds;
                    break;
                case RunPhase::InDuel: {
                    const PlayerTurn t = Sim::choose(run.duel(), run.state().bag, sk, beatFor(run.state().stage, run.state().lap), r);
                    run.duelTurn(t);
                    seconds += kTurnSeconds;
                    ss.seconds += kTurnSeconds;
                    if (run.phase() != RunPhase::InDuel) {
                        const bool won  = run.lastDuel().outcome == DuelOutcome::Won;
                        const bool boss = run.lastDuel().boss;
                        ++ss.duels;
                        ss.darkDuels += run.duel().setup().dark ? 1u : 0u;
                        ss.windDuels += run.duel().setup().weather == Weather::Wind ? 1u : 0u;
                        if (won) {
                            ++ss.duelsWon;
                        } else {
                            ++ss.duelsLost;
                        }
                        if (!boss) {
                            (won ? ss.regularWon : ss.regularLost) += 1u;
                        } else if (won && bossTriesThisStage[lap][stage] == 1u) {
                            ++ss.bossFirstWins;
                        }
                    }
                } break;
                case RunPhase::JoinOffer: {
                    const RunState& s = run.state();
                    run.acceptJoin(s.partnerCount < kMaxPartners ? kNone : oldestPartner(s));
                    seconds += kOfferSeconds;
                    ss.seconds += kOfferSeconds;
                } break;
                case RunPhase::StageClear: {
                    ++ss.cleared;
                    const uint16_t clock = static_cast<uint16_t>(run.state().minutes % (24 * 60));
                    ss.clockSum += clock;
                    ss.darkEnds += timeOfDay(run.state().minutes) == TimeOfDay::Day ? 0u : 1u;
                    run.nextStage();
                } break;
                case RunPhase::Finished:
                    if (run.state().lap + 1u < laps) {
                        run.startNextLap();
                    } else {
                        done = true;
                    }
                    break;
            }
            if (run.wantsSave()) {
                run.saved();
            }
            if (done) {
                ++finished;
                partnersAtEnd += run.state().partnerCount;
                levelAtEnd    += run.state().level;
                break;
            }
        }
        totalSeconds += seconds;
    }

    char out[12288];
    int  len = std::snprintf(out, sizeof(out),
        "%u runs, %s skill: %u reached the %s (%u %%), level %u.%u at the end, %u.%u partners.\n"
        "Play time for %s: %u min on average.\n\n",
        runs, sk.name, finished, laps > 1 ? "end of the last lap" : "summit", finished * 100u / runs,
        finished ? levelAtEnd / finished : 0u, finished ? (levelAtEnd * 10u / finished) % 10u : 0u,
        finished ? partnersAtEnd / finished : 0u, finished ? (partnersAtEnd * 10u / finished) % 10u : 0u,
        laps > 1 ? "all the laps" : "the whole Ridge", totalSeconds / runs / 60u);
    for (uint32_t lap = 0; lap < laps; ++lap) {
        if (laps > 1 && len > 0 && static_cast<size_t>(len) < sizeof(out)) {
            len += std::snprintf(out + len, sizeof(out) - static_cast<size_t>(len), "**Lap %u**\n\n", lap + 1u);
        }
        if (len > 0 && static_cast<size_t>(len) < sizeof(out)) {
            len += std::snprintf(out + len, sizeof(out) - static_cast<size_t>(len),
                "| Stage | Par at the boss | Level at the boss (mean, range) | Duels won / lost | Regular duels won | Boss attempts | Boss first time | In the dark / wind | Play time |\n"
                "|---|---|---|---|---|---|---|---|---|\n");
        }
        for (uint8_t i = 0; i < kStageCount && len > 0 && static_cast<size_t>(len) < sizeof(out); ++i) {
            const StageStats& s = st[lap][i];
            const uint32_t reached = s.reached ? s.reached : 1u;
            const uint32_t regular = s.regularWon + s.regularLost ? s.regularWon + s.regularLost : 1u;
            const uint32_t duels   = s.duels ? s.duels : 1u;
            const uint32_t par     = kStages[i].parLevel + 1u + kLapLevelStep * lap;
            len += std::snprintf(out + len, sizeof(out) - static_cast<size_t>(len),
                "| %u %s | %u | %u.%u (%u-%u) | %u.%u / %u.%u | %u %% | %u.%u | %u %% | %u %% / %u %% | %u min |\n",
                i + 1u, kStages[i].name, par,
                s.levelSum / reached, (s.levelSum * 10u / reached) % 10u, s.levelMin == 255 ? 0u : s.levelMin, s.levelMax,
                s.duelsWon / runs, (s.duelsWon * 10u / runs) % 10u, s.duelsLost / runs, (s.duelsLost * 10u / runs) % 10u,
                s.regularWon * 100u / regular,
                s.bossTries / reached, (s.bossTries * 10u / reached) % 10u,
                s.bossFirstWins * 100u / reached,
                s.darkDuels * 100u / duels, s.windDuels * 100u / duels,
                s.seconds / runs / 60u);
        }
        if (len > 0 && static_cast<size_t>(len) < sizeof(out)) {
            len += std::snprintf(out + len, sizeof(out) - static_cast<size_t>(len), "\n");
        }
    }
    std::fputs(out, stdout);
    if (md != nullptr) {
        if (FILE* f = std::fopen(md, "w")) {
            std::fputs(out, f);
            std::fclose(f);
        }
    }
    return 0;
}
