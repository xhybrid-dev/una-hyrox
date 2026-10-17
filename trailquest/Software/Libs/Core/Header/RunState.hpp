/**
 ******************************************************************************
 * @file    RunState.hpp
 * @brief   Everything about a game in progress that the save file keeps.
 *
 * Header-only (the GUI shows it). Plain integers in fixed-size arrays. A duel
 * in progress is not part of it: the game saves at huts and stage ends, so a
 * save is always taken standing on the map.
 ******************************************************************************
 */

#ifndef TRAILQUEST_RUN_STATE_HPP
#define TRAILQUEST_RUN_STATE_HPP

#include <cstdint>

#include "Progress.hpp"
#include "Types.hpp"
#include "data/Items.hpp"
#include "data/Stages.hpp"

namespace TrailQuest
{

constexpr uint8_t kMaxPartners = 3;
constexpr uint8_t kMaxLap      = 9;

struct RunState {
    uint32_t  rng          = 1;
    uint8_t   level        = 1;
    uint16_t  xp           = 0;    ///< Towards the next level.
    uint16_t  stamina      = 0;    ///< Now.
    SpeciesId partners[kMaxPartners] = { kNone, kNone, kNone };
    uint8_t   partnerCount = 0;    ///< R1's Technique uses the one best placed for each spirit.
    uint8_t   bag[Item::Count] = {};
    uint8_t   stage        = 0;    ///< 0-4.
    uint8_t   node         = 0;
    uint16_t  minutes      = kRunStartMinutes;   ///< The trail clock (runs on across days).
    uint8_t   weather      = 0;    ///< Weather, for this stage.
    uint8_t   lap          = 0;    ///< 0 on the first crossing of the Ridge.
    uint8_t   hutNode      = 0;    ///< Where a loss returns you, in this stage.
    uint8_t   bossesBeaten = 0;    ///< A bit per stage, this lap.
    uint8_t   cachesTaken  = 0;    ///< A bit per stage, this lap.
    uint32_t  spiritsMet    = 0;   ///< A bit per species, ever.
    uint32_t  spiritsBeaten = 0;   ///< A bit per species, ever.
    uint16_t  duelsWon     = 0;
    uint16_t  duelsLost    = 0;
    uint16_t  perfects     = 0;    ///< Perfect beats, ever.
};

static_assert(Spirit::Count <= 32, "spiritsMet and spiritsBeaten hold one bit per species");
static_assert(kStageCount <= 8, "bossesBeaten and cachesTaken hold one bit per stage");

/// The runner's stats now: the level's, plus a bonus for each partner.
inline Stats runnerMaxWithPartners(const RunState& s)
{
    Stats st = runnerStats(s.level);
    for (uint8_t i = 0; i < s.partnerCount && i < kMaxPartners; ++i) {
        switch (speciesDef(s.partners[i]).affinity) {
            case Affinity::Pace: st.pace = static_cast<uint8_t>(st.pace + kPartnerBonus); break;
            case Affinity::Form: st.form = static_cast<uint8_t>(st.form + kPartnerBonus); break;
            case Affinity::Grit: st.grit = static_cast<uint8_t>(st.grit + kPartnerBonus); break;
            default: break;
        }
    }
    return st;
}

/// Consumables in the bag (key items not counted).
inline uint8_t bagCount(const RunState& s)
{
    uint32_t n = 0;
    for (ItemId i = 0; i < Item::Count; ++i) {
        if (!itemDef(i).key) {
            n += s.bag[i];
        }
    }
    return static_cast<uint8_t>(n > 255u ? 255u : n);
}

inline bool hasPartner(const RunState& s, SpeciesId sp)
{
    for (uint8_t i = 0; i < s.partnerCount && i < kMaxPartners; ++i) {
        if (s.partners[i] == sp) {
            return true;
        }
    }
    return false;
}

} // namespace TrailQuest

#endif // TRAILQUEST_RUN_STATE_HPP
