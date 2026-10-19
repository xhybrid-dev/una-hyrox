/**
 ******************************************************************************
 * @file    Story.hpp
 * @brief   The story: what the game tells the runner between the steps.
 *
 * Header-only data (the GUI shows it). Short pages at the key moments only
 * (Gate Q2: "a bit hard to follow the journey"):
 *   - the opening tale, before a new game: the Ridge, the spirits, a duel,
 *     the wheel, the pack, the map;
 *   - as each stage starts: the place, then its boss, the goal;
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
    { "THE RIDGE", "Five stages from valley to summit. You, in new shoes.", kNone },
    { "SPIRITS",   "Wild things live up here. Each one wants a race.", Spirit::RedFox },
    { "DUELS",     "Pick a move, then press as the needle hits the green.", Spirit::BrownBear },
    { "THE WHEEL", "Pace beats\nForm beats\nGrit beats\nPace.", Spirit::SandCrab },
    { "YOUR PACK", "A spirit you beat may join you and lend its move.", Spirit::RubyBeetle },
    { "THE MAP",   "Pick a way at each fork. Hold R2 to stop any time.", kNone },
};

inline constexpr StoryPage kStageStart[kStageCount][kStageStartPages] = {
    { { "STAGE 1", "Valley Path. Soft ground, a stream. Easy, mostly.", kNone },
      { "AHEAD",   "The Old Tortoise guards the end. Beat it to go on.", Spirit::OldTortoise } },
    { { "STAGE 2", "Switchback Climb. Zig, zag, zig. Your calves have notes.", kNone },
      { "AHEAD",   "The Great Stag waits at the top. It isn't tired.", Spirit::GreatStag } },
    { { "STAGE 3", "Scree Fields. Every stone has an opinion.", kNone },
      { "AHEAD",   "The Ram King rules the scree. It decides who passes.", Spirit::RamKing } },
    { { "STAGE 4", "Ridge Line. Wind on both sides, and a drop on both sides.", kNone },
      { "AHEAD",   "The Golden Eagle owns this edge. Keep your feet.", Spirit::GoldenEagle } },
    { { "STAGE 5", "The Summit. Snow, thin air, and quiet.", kNone },
      { "AHEAD",   "Something has followed you since the valley.", Spirit::SummitWolf } },
};

inline constexpr StoryPage kBossAhead[kStageCount] = {
    { "THE POOL",  "It has waited since breakfast. Last Tuesday's.", Spirit::OldTortoise },
    { "THE CRAG",  "Antlers on the sky. Not even out of breath.", Spirit::GreatStag },
    { "THE LEDGE", "Stone on stone. The Ram King lowers its horns.", Spirit::RamKing },
    { "THE EDGE",  "Wings wide on the wind. It has watched you all day.", Spirit::GoldenEagle },
    { "THE TOP",   "The Summit Wolf. It has run beside you all along.", Spirit::SummitWolf },
};

inline constexpr StoryPage kBossBeaten[kStageCount] = {
    { "THE POOL",  "The tortoise nods, slowly. The way up is open.", Spirit::OldTortoise },
    { "THE CRAG",  "The stag steps aside. Below you, the valley looks tiny.", Spirit::GreatStag },
    { "THE LEDGE", "The ram stands aside. The ridge is above you.", Spirit::RamKing },
    { "THE EDGE",  "The eagle lifts away. Only the summit is left.", Spirit::GoldenEagle },
    { "THE TOP",   "The wolf sits by you. Below, the whole Ridge. You ran it.", Spirit::SummitWolf },
};

/// One story's pages, in order.
struct Story {
    const StoryPage* pages = nullptr;
    uint8_t          count = 0;
};

/// The story @p id for @p stage (0-4; the first for one out of range).
inline Story story(StoryId id, uint8_t stage)
{
    const uint8_t s = stage < kStageCount ? stage : 0u;
    switch (id) {
        case StoryId::Opening:
            return { kOpening, static_cast<uint8_t>(sizeof(kOpening) / sizeof(kOpening[0])) };
        case StoryId::StageStart:
            return { kStageStart[s], kStageStartPages };
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

    /// One story.
    static StoryPlan of(StoryId id, uint8_t stage)
    {
        StoryPlan p;
        p.parts[0]  = id;
        p.partCount = 1;
        p.stage     = stage;
        return p;
    }

    /// Two in a row.
    static StoryPlan of(StoryId first, StoryId second, uint8_t stage)
    {
        StoryPlan p = of(first, stage);
        p.parts[1]  = second;
        p.partCount = 2;
        return p;
    }

    uint8_t pageCount() const
    {
        uint8_t n = 0;
        for (uint8_t i = 0; i < partCount && i < kMaxParts; ++i) {
            n = static_cast<uint8_t>(n + story(parts[i], stage).count);
        }
        return n;
    }

    /// Page @p i of the whole run, or nullptr past its end.
    const StoryPage* page(uint8_t i) const
    {
        for (uint8_t k = 0; k < partCount && k < kMaxParts; ++k) {
            const Story s = story(parts[k], stage);
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
