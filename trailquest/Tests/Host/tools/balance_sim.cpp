// balance_sim: every Trail Spirit at its stage's par, many seeded duels per
// skill profile, against the targets in docs/BALANCE.md.
//
//   balance_sim [duels-per-row] [--md out.md] [--csv out.csv]
//
// The runner: at the stage's par level for a regular spirit, one level above
// for the boss (the stage's duels raise it). On the first stage alone; from
// the second with a pack of three partners from earlier stages (one of each
// affinity, the most recent); R1 plays the partner best placed against the spirit,
// as the game does (Run::bestTechnique). A gel and a water in the bag (two gels for a
// boss), full stamina. Each duel in one of the stage's two weathers, half and
// half, as the game picks them (Run::startStage); never in the dark (run_sim
// reports how often a run is). The player: Tests/Host/tools/Policy.hpp.
//
// Targets (proposed at Gate Q1, docs/BALANCE.md): a regular spirit at par,
// casual skill, 80-95 % won in 3-6 turns on average; a boss, met fresh, 65-85 %
// (a real arrival is tired: run_sim shows about half beaten first time).

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Encounter.hpp"
#include "Policy.hpp"
#include "Progress.hpp"
#include "RunState.hpp"
#include "data/Stages.hpp"

using namespace TrailQuest;

namespace
{

struct Row {
    SpeciesId sp;
    uint8_t   stage;
    uint8_t   runnerLevel;
    bool      pack;
    uint32_t  won[Sim::kSkillCount];
    uint32_t  turns[Sim::kSkillCount];
    uint32_t  leftPct[Sim::kSkillCount];   // stamina left on a win, summed
    uint32_t  soloWon;                     // casual, no partners
};

/// A pack from earlier stages: the most recent regular spirit of each affinity.
uint8_t packBefore(uint8_t stage, SpeciesId out[kMaxPartners])
{
    uint8_t n = 0;
    for (uint8_t a = 0; a < kAffinityCount && n < kMaxPartners; ++a) {
        for (int st = static_cast<int>(stage) - 1; st >= 0; --st) {
            bool found = false;
            for (uint8_t i = 0; i < kStages[st].poolCount; ++i) {
                const SpeciesId sp = kStages[st].pool[i];
                if (sp < Spirit::Count && static_cast<uint8_t>(kSpecies[sp].affinity) == a) {
                    out[n++] = sp;
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
    }
    return n;
}

DuelSetup setup(const RunState& rs, const StageDef& stage, SpeciesId sp, uint8_t level, MoveId technique,
                Weather weather)
{
    DuelSetup s;
    s.runnerLevel   = rs.level;
    s.runnerMax     = runnerMaxWithPartners(rs);
    s.runnerStamina = s.runnerMax.stamina;
    s.technique     = technique;
    s.species       = sp;
    s.speciesLevel  = level;
    s.terrain       = stage.terrain;
    s.weather       = weather;
    return s;
}

/// The partner whose move does most against @p sp (Run::bestTechnique's rule).
MoveId bestTechnique(const RunState& rs, const StageDef& stage, SpeciesId sp, uint8_t level, Weather weather)
{
    MoveId   best    = Move::TrailCraft;
    uint16_t bestDmg = 0;
    for (uint8_t i = 0; i < rs.partnerCount; ++i) {
        const MoveId m = signatureMove(rs.partners[i]);
        Duel d;
        d.start(setup(rs, stage, sp, level, m, weather));
        const uint16_t dmg = d.estimate(Action::Technique);
        if (dmg > bestDmg) {
            bestDmg = dmg;
            best    = m;
        }
    }
    Duel d;
    d.start(setup(rs, stage, sp, level, Move::TrailCraft, weather));
    return d.estimate(Action::Technique) > bestDmg ? static_cast<MoveId>(Move::TrailCraft) : best;
}

bool fight(const RunState& rs, const StageDef& stage, SpeciesId sp, uint8_t level, MoveId technique,
           Weather weather, const Sim::Skill& skill, Rng& r, uint8_t& turns, uint32_t& leftPct)
{
    Duel d;
    d.start(setup(rs, stage, sp, level, technique, weather));
    uint8_t bag[Item::Count] = {};
    bag[Item::Gel]   = isBoss(sp) ? 2 : 1;
    bag[Item::Water] = 1;
    while (d.outcome() == DuelOutcome::Ongoing) {
        d.play(Sim::choose(d, bag, skill, r), r, bag);
    }
    turns   = d.turn();
    leftPct = static_cast<uint32_t>(d.runner().stamina) * 100u / d.runner().max.stamina;
    return d.outcome() == DuelOutcome::Won;
}

const char* flag(const Row& row, uint32_t n)
{
    const uint32_t pct   = row.won[0] * 100u / n;
    const uint32_t turnsX10 = row.turns[0] * 10u / n;
    if (isBoss(row.sp)) {
        return (pct < 65 || pct > 85) ? "OFF" : "ok";
    }
    if (pct < 80 || pct > 95) {
        return "OFF";
    }
    return (turnsX10 < 30 || turnsX10 > 60) ? "LONG/SHORT" : "ok";
}

} // namespace

int main(int argc, char** argv)
{
    uint32_t    n   = 10000;
    const char* md  = nullptr;
    const char* csv = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--md") == 0 && i + 1 < argc) {
            md = argv[++i];
        } else if (std::strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv = argv[++i];
        } else {
            n = static_cast<uint32_t>(std::strtoul(argv[i], nullptr, 10));
        }
    }
    if (n == 0) {
        n = 10000;
    }

    Row      rows[Spirit::Count + kStageCount];
    uint32_t rowCount = 0;

    for (uint8_t st = 0; st < kStageCount; ++st) {
        const StageDef& stage = kStages[st];
        SpeciesId list[kMaxPool + 1];
        uint8_t   count = 0;
        for (uint8_t i = 0; i < stage.poolCount; ++i) {
            list[count++] = stage.pool[i];
        }
        list[count++] = stage.boss;

        for (uint8_t i = 0; i < count; ++i) {
            const SpeciesId sp = list[i];
            Row row {};
            row.sp    = sp;
            row.stage = st;
            row.pack  = st > 0;

            RunState rs;
            rs.level = static_cast<uint8_t>(stage.parLevel + (isBoss(sp) ? 1 : 0));
            if (row.pack) {
                rs.partnerCount = packBefore(st, rs.partners);
            }
            row.runnerLevel = rs.level;
            RunState solo   = rs;
            solo.partnerCount = 0;

            for (int k = 0; k < Sim::kSkillCount; ++k) {
                Rng r(0xBA1A0000u + sp * 97u + static_cast<uint32_t>(k));
                for (uint32_t j = 0; j < n; ++j) {
                    const uint8_t level   = isBoss(sp) ? bossLevel(stage, 0) : pickLevel(stage, 0, r);
                    const Weather weather = stage.weather[j & 1u];
                    const MoveId  tech    = bestTechnique(rs, stage, sp, level, weather);
                    uint8_t  turns = 0;
                    uint32_t left  = 0;
                    if (fight(rs, stage, sp, level, tech, weather, Sim::kSkills[k], r, turns, left)) {
                        ++row.won[k];
                        row.leftPct[k] += left;
                    }
                    row.turns[k] += turns;
                }
            }
            {
                Rng r(0x5010u + sp);
                for (uint32_t j = 0; j < n; ++j) {
                    const uint8_t level = isBoss(sp) ? bossLevel(stage, 0) : pickLevel(stage, 0, r);
                    uint8_t  turns = 0;
                    uint32_t left  = 0;
                    row.soloWon += fight(solo, stage, sp, level, Move::TrailCraft, stage.weather[j & 1u], Sim::kSkills[0], r,
                                         turns, left) ? 1u : 0u;
                }
            }
            rows[rowCount++] = row;
        }
    }

    // Plain table to stdout.
    std::printf("%-16s st lvl kit  | casual        | good          | menu          | solo  | check\n", "spirit");
    for (uint32_t i = 0; i < rowCount; ++i) {
        const Row& r = rows[i];
        std::printf("%-16s %u  %2u  %-4s |", kSpecies[r.sp].name, r.stage + 1u, r.runnerLevel, r.pack ? "pack" : "solo");
        for (int k = 0; k < Sim::kSkillCount; ++k) {
            std::printf(" %3u%% %2u.%u t  |", r.won[k] * 100u / n, r.turns[k] / n, (r.turns[k] * 10u / n) % 10u);
        }
        std::printf(" %3u%% | %s\n", r.soloWon * 100u / n, flag(r, n));
    }

    if (csv != nullptr) {
        if (FILE* f = std::fopen(csv, "w")) {
            std::fprintf(f, "spirit,stage,runner_level,kit,boss");
            for (int k = 0; k < Sim::kSkillCount; ++k) {
                std::fprintf(f, ",%s_win_pct,%s_turns_x10,%s_left_pct", Sim::kSkills[k].name, Sim::kSkills[k].name, Sim::kSkills[k].name);
            }
            std::fprintf(f, ",solo_casual_win_pct,check\n");
            for (uint32_t i = 0; i < rowCount; ++i) {
                const Row& r = rows[i];
                std::fprintf(f, "%s,%u,%u,%s,%u", kSpecies[r.sp].name, r.stage + 1u, r.runnerLevel, r.pack ? "pack" : "solo",
                             isBoss(r.sp) ? 1u : 0u);
                for (int k = 0; k < Sim::kSkillCount; ++k) {
                    std::fprintf(f, ",%u,%u,%u", r.won[k] * 100u / n, r.turns[k] * 10u / n,
                                 r.won[k] ? r.leftPct[k] / r.won[k] : 0u);
                }
                std::fprintf(f, ",%u,%s\n", r.soloWon * 100u / n, flag(r, n));
            }
            std::fclose(f);
        }
    }

    if (md != nullptr) {
        if (FILE* f = std::fopen(md, "w")) {
            std::fprintf(f, "| Spirit | Stage | Runner | Casual: won, turns | Good | Menu-only | Solo, casual | Check |\n");
            std::fprintf(f, "|---|---|---|---|---|---|---|---|\n");
            for (uint32_t i = 0; i < rowCount; ++i) {
                const Row& r = rows[i];
                std::fprintf(f, "| %s%s | %u | L%u %s |", kSpecies[r.sp].name, isBoss(r.sp) ? " (boss)" : "", r.stage + 1u,
                             r.runnerLevel, r.pack ? "with pack" : "alone");
                for (int k = 0; k < Sim::kSkillCount; ++k) {
                    std::fprintf(f, " %u %%, %u.%u |", r.won[k] * 100u / n, r.turns[k] / n, (r.turns[k] * 10u / n) % 10u);
                }
                std::fprintf(f, " %u %% | %s |\n", r.soloWon * 100u / n, flag(r, n));
            }
            std::fclose(f);
        }
    }
    return 0;
}
