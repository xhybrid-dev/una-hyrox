/**
 ******************************************************************************
 * @file    Story.hpp
 * @brief   The story: what the game tells the runner between the steps.
 *
 * Header-only data (the GUI shows it). Short pages at the key moments only
 * (Gate Q2: "a bit hard to follow the journey"):
 *   - the opening tale, before a new game: the Ridge, the spirits, a duel,
 *     the wheel, the pack, the map;
 *   - as each stage starts: the place, then its boss, the goal (the first
 *     stage has its own pages from the second lap on);
 *   - on arriving at the boss, and when it is beaten.
 *
 * A page is a headline (at most Layout::kHeadCols, in capitals) and a body
 * that wraps to the story card (Layout::kStoryLines of kStoryCols); the host
 * tests check every page fits (LayoutTest). Funny start, serious summit
 * (DESIGN.md, "Voice"); British English.
 ******************************************************************************
 */

#ifndef TRAILQUEST_DATA_STORY_HPP
#define TRAILQUEST_DATA_STORY_HPP

#include <cstdint>

#include "Types.hpp"
#include "data/Species.hpp"
#include "data/Stages.hpp"

namespace TrailQuest
{

struct StoryPage {
    const char* head;   ///< The headline (Layout::kHeadRow).
    const char* body;   ///< Wrapped to the story card; '\n' breaks a line.
    SpeciesId   who;    ///< Who stands with the runner in the scene (kNone: the runner alone).
};

enum class StoryId : uint8_t {
    Opening,      ///< Before a new game.
    StageStart,   ///< As a stage starts: the place, then its boss.
    BossAhead,    ///< On arriving at the stage's boss.
    BossBeaten,   ///< When it is beaten.
};

/// Pages a stage has as it starts.
constexpr uint8_t kStageStartPages = 2;

inline constexpr StoryPage kOpening[] = {
    { "THE RIDGE", "Five stages, valley to summit. Pace yourself.", kNone },
    { "SPIRITS",   "Spirits are trail troubles: stitch, bonk, cramp.", Spirit::RedFox },
    { "DUELS",     "Pick a move. Press when the needle is in the green.", Spirit::BrownBear },
    { "THE WHEEL", "Pace beats\nForm beats\nGrit beats\nPace.", Spirit::SandCrab },
    { "ENERGY",    "Running low? Take a Gel before you hit the wall.", kNone },
    { "CRAMP",     "Cramp slows you: take a Salt tab. Drained: Water.", kNone },
    { "YOUR PACK", "A beaten spirit may join. You pick whose move to use.", Spirit::RubyBeetle },
    { "THE MAP",   "Pick a route at each fork. Huts restore you. Auto-saves.", kNone },
};

inline constexpr StoryPage kStageStart[kStageCount][kStageStartPages] = {
    { { "STAGE 1", "Valley Path. Easy ground. Start steady and eat early.", kNone },
      { "AHEAD",   "The Old Tortoise is fatigue. Fuel up first.", Spirit::OldTortoise } },
    { { "STAGE 2", "Switchback Climb. Steep. Pace it, save energy.", kNone },
      { "AHEAD",   "The Great Stag is the long climb. Take a Gel before it.", Spirit::GreatStag } },
    { { "STAGE 3", "Scree Fields. Loose rock. Short steps, eyes ahead.", kNone },
      { "AHEAD",   "The Ram King is the scree itself. Poles help on rock.", Spirit::RamKing } },
    { { "STAGE 4", "Ridge Line. Wind and drops. Keep a short, low stride.", kNone },
      { "AHEAD",   "The Golden Eagle is the exposed edge. Stay steady.", Spirit::GoldenEagle } },
    { { "STAGE 5", "The Summit. Cold, thin air. Eat, drink, keep moving.", kNone },
      { "AHEAD",   "The grey shape from the ridge waits at the top.", Spirit::SummitWolf } },
};

/// The first stage's pages from the second lap on (the place, then its boss).
inline constexpr StoryPage kLapStart[kStageStartPages] = {
    { "NEXT LAP", "Same valley, tougher spirits. Eat early.", kNone },
    { "AHEAD",    "The Old Tortoise is back. Fuel up before it.", Spirit::OldTortoise },
};

inline constexpr StoryPage kBossAhead[kStageCount] = {
    { "THE POOL",  "The Old Tortoise blocks you. It guards at times.", Spirit::OldTortoise },
    { "THE CRAG",  "The Great Stag lowers its antlers. Settle to brace.", Spirit::GreatStag },
    { "THE LEDGE", "The Ram King holds the ledge. Its guard halves your hits.", Spirit::RamKing },
    { "THE EDGE",  "The Golden Eagle dives from above. Settle to brace.", Spirit::GoldenEagle },
    { "THE TOP",   "The Summit Wolf blocks the top. It charges, then guards.", Spirit::SummitWolf },
};

inline constexpr StoryPage kBossBeaten[kStageCount] = {
    { "THE POOL",  "Valley done. The way up is open. Eat before the climb.", Spirit::OldTortoise },
    { "THE CRAG",  "Climb done. Something grey watches from afar.", Spirit::GreatStag },
    { "THE LEDGE", "Scree done. Something grey keeps pace.", Spirit::RamKing },
    { "THE EDGE",  "The edge is behind you. The grey shape is closer.", Spirit::GoldenEagle },
    { "THE TOP",   "The wolf sits by you. Below, the whole Ridge. You ran it.", Spirit::SummitWolf },
};

/// One story's pages, in order.
struct Story {
    const StoryPage* pages = nullptr;
    uint8_t          count = 0;
};

/// The story @p id for @p stage (0-4; the first for one out of range). On a
/// later @p lap the first stage's start is the lap's own.
inline Story story(StoryId id, uint8_t stage, uint8_t lap = 0)
{
    const uint8_t s = stage < kStageCount ? stage : 0u;
    switch (id) {
        case StoryId::Opening:
            return { kOpening, static_cast<uint8_t>(sizeof(kOpening) / sizeof(kOpening[0])) };
        case StoryId::StageStart:
            return { lap > 0 && s == 0 ? kLapStart : kStageStart[s], kStageStartPages };
        case StoryId::BossAhead:
            return { &kBossAhead[s], 1 };
        case StoryId::BossBeaten:
            return { &kBossBeaten[s], 1 };
    }
    return {};
}

/// What one story screen plays: up to two stories in a row, as one run of
/// pages (a new game's opening, then its first stage's start).
struct StoryPlan {
    static constexpr uint8_t kMaxParts = 2;

    StoryId parts[kMaxParts] = { StoryId::Opening, StoryId::Opening };
    uint8_t partCount        = 0;
    uint8_t stage            = 0;
    uint8_t lap              = 0;

    /// One story.
    static StoryPlan of(StoryId id, uint8_t stage, uint8_t lap = 0)
    {
        StoryPlan p;
        p.parts[0]  = id;
        p.partCount = 1;
        p.stage     = stage;
        p.lap       = lap;
        return p;
    }

    /// Two in a row.
    static StoryPlan of(StoryId first, StoryId second, uint8_t stage, uint8_t lap = 0)
    {
        StoryPlan p = of(first, stage, lap);
        p.parts[1]  = second;
        p.partCount = 2;
        return p;
    }

    uint8_t pageCount() const
    {
        uint8_t n = 0;
        for (uint8_t i = 0; i < partCount && i < kMaxParts; ++i) {
            n = static_cast<uint8_t>(n + story(parts[i], stage, lap).count);
        }
        return n;
    }

    /// Page @p i of the whole run, or nullptr past its end.
    const StoryPage* page(uint8_t i) const
    {
        for (uint8_t k = 0; k < partCount && k < kMaxParts; ++k) {
            const Story s = story(parts[k], stage, lap);
            if (i < s.count) {
                return &s.pages[i];
            }
            i = static_cast<uint8_t>(i - s.count);
        }
        return nullptr;
    }
};

} // namespace TrailQuest

#endif // TRAILQUEST_DATA_STORY_HPP
