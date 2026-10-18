// Everything the game says fits where it is drawn on the round face: each row
// holds its column count, and every name, label, and narrated line (over all
// spirits, moves, items, places and events) fits its block.

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>

#include "Layout.hpp"
#include "Narrate.hpp"
#include "TextFit.hpp"

using namespace TrailQuest;
using namespace TrailQuest::Layout;
using TextFit::Lines;

namespace
{

size_t len(const char* s) { return std::strlen(s); }

void expectFits(const Lines& l, uint8_t maxLines, uint8_t cols, const char* what)
{
    EXPECT_TRUE(l.fits) << what;
    EXPECT_LE(l.count, maxLines) << what;
    for (uint8_t i = 0; i < l.count; ++i) {
        // A card's title line is a little narrower (Layout.hpp).
        const uint8_t c = (cols == kCardCols && i == 0) ? kCardTitleCols : cols;
        EXPECT_LE(len(l.text[i]), static_cast<size_t>(c)) << what << ": " << l.text[i];
    }
}

/// A world standing at @p node of @p stage.
WorldView worldAt(uint8_t stage, uint8_t node)
{
    WorldView w;
    w.state.stage   = stage;
    w.state.node    = node;
    w.state.level   = 20;
    w.state.minutes = 23 * 60 + 59;
    w.hasGame       = 1;
    return w;
}

} // namespace

TEST(Layout, EachRowHoldsItsColumns)
{
    EXPECT_GE(TextFit::colsAt(kHeadRow), kHeadCols);
    EXPECT_GE(TextFit::colsOver(kCardRow, kCardLines, kCardPitch), kCardCols);
    EXPECT_GE(TextFit::colsOver(kNarrRow, kNarrLines, kNarrPitch), kNarrCols);
    EXPECT_GE(TextFit::colsAt(kExitRow), kExitCols);
    EXPECT_GE(TextFit::colsAt(kFootRow), kFootCols);
    EXPECT_GE(TextFit::colsAt(kMapClockRow), kMapClockCols);
    // The blocks keep out of each other's way.
    EXPECT_LE(kCardRow + (kCardLines - 1) * kCardPitch + TextFit::kGlyph, kBottomLabelRow);
    EXPECT_GE(kCardRow, kTopLabelRow + TextFit::kGlyph);
    EXPECT_LE(kBottomLabelRow + TextFit::kGlyph, kExitRow);
    EXPECT_LE(kMapLabelRow + TextFit::kGlyph, kMapTop);
    EXPECT_LE(kMapClockRow + TextFit::kGlyph, kMapLabelRow);
    // The map's foot: the way's name, what lies that way, the stamina bar.
    EXPECT_GE(TextFit::colsAt(kLeadsRow), kLeadsCols);
    EXPECT_LE(kMapBottom, kExitRow);
    EXPECT_LE(kExitRow + TextFit::kGlyph, kLeadsRow);
    EXPECT_LE(kLeadsRow + TextFit::kGlyph, kMapBarRow);
    // A story page: its body on the card's rows, the scene below them.
    EXPECT_GE(TextFit::colsOver(kStoryRow, kStoryLines, kCardPitch), kStoryCols);
    EXPECT_LE(kStoryRow + (kStoryLines - 1) * kCardPitch + TextFit::kGlyph, kSceneTop);
    EXPECT_LT(kSceneTop, kSceneGround);
}


namespace
{

/// The bezel arcs as the SDK draws them (Buttons.cpp): centre-line radius 113,
/// 6 px wide, and these spans in TouchGFX degrees (0 at 12 o'clock, clockwise)
/// for L1, L2, R1, R2. Widened here by the rounded caps and a little margin.
constexpr double kArcSpans[4][2] = { { 291, 308 }, { 232, 249 }, { 52, 69 }, { 112, 128 } };

/// True if no point of the box [x0, x1) x [y0, y1) lies on a bezel arc.
bool clearOfArcs(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    for (int32_t y = y0; y <= y1; y += 2) {
        for (int32_t x = x0; x <= x1; x += 2) {
            if (y != y0 && y != y1 && x != x0 && x != x1 && x + 2 <= x1 && y + 2 <= y1) {
                continue;   // only the edges can reach an arc first
            }
            const double dx = x - 120.0;
            const double dy = y - 120.0;
            const double r  = std::sqrt(dx * dx + dy * dy);
            if (r < 113.0 - 3.0 - 2.0) {
                continue;
            }
            double deg = std::atan2(dx, -dy) * 180.0 / 3.14159265358979;
            if (deg < 0) {
                deg += 360.0;
            }
            for (const auto& span : kArcSpans) {
                if (deg >= span[0] - 3.0 && deg <= span[1] + 3.0) {
                    return false;
                }
            }
        }
    }
    return true;
}

/// A centred line of @p cols characters at @p row is clear of the arcs.
bool centredClear(int32_t row, uint8_t cols)
{
    const int32_t w = cols * TextFit::kGlyph;
    return clearOfArcs(120 - w / 2, row, 120 + w / 2, row + TextFit::kGlyph);
}

bool insideArcs(int32_t x, int32_t y)
{
    const int32_t dx = x - TextFit::kRadius;
    const int32_t dy = y - TextFit::kRadius;
    return dx * dx + dy * dy <= kArcClear * kArcClear;
}

template <size_t L, size_t R>
void expectCornersFit(int32_t row, int32_t left, int32_t right, const char* const (&ls)[L], const char* const (&rs)[R])
{
    size_t widestLeft = 0;
    size_t widestRight = 0;
    for (const char* l : ls) {
        const int32_t w = static_cast<int32_t>(len(l)) * TextFit::kGlyph;
        widestLeft = len(l) > widestLeft ? len(l) : widestLeft;
        // A left word's box, from its start to its end, inside the arcs.
        EXPECT_TRUE(insideArcs(left, row) && insideArcs(left, row + TextFit::kGlyph)) << l;
        EXPECT_TRUE(insideArcs(left + w, row) && insideArcs(left + w, row + TextFit::kGlyph)) << l;
    }
    for (const char* r : rs) {
        const int32_t w = static_cast<int32_t>(len(r)) * TextFit::kGlyph;
        widestRight = len(r) > widestRight ? len(r) : widestRight;
        EXPECT_TRUE(insideArcs(right, row) && insideArcs(right, row + TextFit::kGlyph)) << r;
        EXPECT_TRUE(insideArcs(right - w, row) && insideArcs(right - w, row + TextFit::kGlyph)) << r;
    }
    // The widest pair on one row does not meet in the middle.
    EXPECT_LE(static_cast<int32_t>(widestLeft + widestRight) * TextFit::kGlyph + 8, right - left) << row;
}

} // namespace

TEST(Layout, EveryButtonWordFitsInsideItsArc)
{
    using namespace Narrate::Label;
    expectCornersFit(kTopLabelRow, kTopLabelLeft, kTopLabelRight, kTopLeft, kTopRight);
    expectCornersFit(kBottomLabelRow, kBottomLabelLeft, kBottomLabelRight, kBottomLeft, kBottomRight);
    expectCornersFit(kMapLabelRow, kMapLabelLeft, kMapLabelRight, kMapLeft, kMapRight);
    // The arcs themselves are where the SDK draws them (Buttons.cpp: radius
    // 113, 6 px wide), so 109 leaves a pixel clear of the inner edge.
    EXPECT_LT(kArcClear, 113 - 6 / 2);
}

TEST(Layout, NoLineOfTextRunsIntoAnArc)
{
    EXPECT_TRUE(centredClear(kHeadRow, kHeadCols));
    EXPECT_TRUE(centredClear(kExitRow, kExitCols));
    EXPECT_TRUE(centredClear(kFootRow, kFootCols));
    EXPECT_TRUE(centredClear(kMapClockRow, kMapClockCols));
    // A card's first line is its title (a name, at most kCardTitleCols); the
    // rest may use the full width.
    EXPECT_TRUE(centredClear(kCardRow, kCardTitleCols));
    for (uint8_t i = 1; i < kCardLines; ++i) {
        EXPECT_TRUE(centredClear(kCardRow + i * kCardPitch, kCardCols)) << int(i);
    }
    // The duel's second narration row is shown with the arcs lit (the
    // technique's name); the first only while they are dark.
    EXPECT_TRUE(centredClear(kNarrRow + kNarrPitch, kNarrCols));
    EXPECT_TRUE(centredClear(kLeadsRow, kLeadsCols));
    for (uint8_t i = 0; i < kStoryLines; ++i) {
        EXPECT_TRUE(centredClear(kStoryRow + i * kCardPitch, kStoryCols)) << int(i);
    }
}

TEST(Layout, EveryMoveItemAndExitNameFitsItsRow)
{
    for (MoveId m = 0; m < Move::Count; ++m) {
        EXPECT_LE(len(moveDef(m).name), static_cast<size_t>(kNarrCols)) << moveDef(m).name;
    }
    for (ItemId i = 0; i < Item::Count; ++i) {
        // "+1 <item>" on a card, "<item> x9" in the fuel picker.
        EXPECT_LE(len(itemDef(i).name) + 3, static_cast<size_t>(kNarrCols)) << itemDef(i).name;
    }
    for (const StageDef& st : kStages) {
        for (uint8_t n = 0; n < st.nodeCount; ++n) {
            for (uint8_t e = 0; e < st.nodes[n].exitCount; ++e) {
                EXPECT_LE(len(st.nodes[n].exits[e].label), static_cast<size_t>(kExitCols)) << st.nodes[n].exits[e].label;
            }
        }
    }
    for (SpeciesId s = 0; s < Spirit::Count; ++s) {
        if (!isBoss(s)) {
            // A partner's name on the pack page and the offer screen: one line.
            EXPECT_LE(len(speciesDef(s).name), static_cast<size_t>(kCardCols)) << speciesDef(s).name;
        }
    }
    char clock[8];
    Narrate::clock(clock, 0xFFFF);
    EXPECT_EQ(len(clock), 5u);
    EXPECT_LE(len(clock), static_cast<size_t>(kMapClockCols));
}

TEST(Layout, EverySpiritsIntroFitsACard)
{
    for (SpeciesId s = 0; s < Spirit::Count; ++s) {
        Lines l;
        Narrate::spiritIntro(l, s);
        expectFits(l, kCardLines, kCardCols, speciesDef(s).name);
    }
}

TEST(Layout, EveryEventOfADuelFitsTheNarration)
{
    DuelView d;
    d.runner.level = 20;
    d.spirit.level = 20;
    const EventKind kinds[] = { EventKind::Hit, EventKind::Missed, EventKind::Inflicted, EventKind::Wore,
                                EventKind::Ticked, EventKind::Healed, EventKind::UsedItem, EventKind::NoItem,
                                EventKind::Rested };
    for (EventKind k : kinds) {
        for (MoveId m = 0; m < Move::Count; ++m) {
            for (uint8_t eff = 0; eff < static_cast<uint8_t>(Effect::Count); ++eff) {
                for (ItemId it = 0; it < Item::Count; ++it) {
                    DuelEvent e;
                    e.kind   = k;
                    e.move   = m;
                    e.effect = static_cast<Effect>(eff);
                    e.item   = it;
                    e.amount = 65535;
                    for (Side s : { Side::Runner, Side::Spirit }) {
                        e.actor = s;
                        Lines l;
                        Narrate::event(l, e, d);
                        expectFits(l, kNarrLines, kNarrCols, moveDef(m).name);
                        EXPECT_GE(l.count, 1u);
                    }
                }
            }
        }
    }
}

TEST(Layout, EveryCardOnTheMapFits)
{
    for (uint8_t st = 0; st < kStageCount; ++st) {
        for (uint8_t n = 0; n < kStages[st].nodeCount; ++n) {
            WorldView w = worldAt(st, n);
            for (ViewCause c : { ViewCause::NewGame, ViewCause::StageStarted, ViewCause::Loaded }) {
                for (uint8_t wx = 0; wx < static_cast<uint8_t>(Weather::Count); ++wx) {
                    for (uint8_t t = 0; t < 4; ++t) {
                        w.cause         = c;
                        w.state.weather = wx;
                        w.timeOfDay     = t;
                        Lines l;
                        Narrate::arrival(l, w);
                        expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
                    }
                }
            }
            w.cause = ViewCause::Arrived;
            for (Arrival a : { Arrival::Nothing, Arrival::Found, Arrival::Hut }) {
                for (ItemId it = 0; it < Item::Count; ++it) {
                    for (bool full : { false, true }) {
                        w.arrive.what    = a;
                        w.arrive.item    = it;
                        w.arrive.bagFull = full;
                        Lines l;
                        Narrate::arrival(l, w);
                        expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
                    }
                }
            }
        }
    }
}

TEST(Layout, EveryDuelEndObstacleOfferAndStageCardFits)
{
    for (uint8_t st = 0; st < kStageCount; ++st) {
        for (uint8_t n = 0; n < kStages[st].nodeCount; ++n) {
            const NodeKind k = kStages[st].nodes[n].kind;
            WorldView w = worldAt(st, n);
            if (k == NodeKind::Hut || k == NodeKind::Start) {
                // A loss lands here.
                for (ItemId lost : { kNone, ItemId{Item::Gel}, ItemId{Item::SaltTab}, ItemId{Item::Poles} }) {
                    w.duel.outcome  = DuelOutcome::Lost;
                    w.duel.lostItem = lost;
                    Lines l;
                    Narrate::duelEnd(l, w);
                    expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
                }
            }
            if (k == NodeKind::Obstacle) {
                w.odds[0] = 100;
                w.odds[1] = 100;
                w.odds[2] = 100;
                Lines l;
                Narrate::obstacleIntro(l, w);
                expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
                w.obstacle.cleared     = false;
                w.obstacle.staminaLost = 999;
                w.obstacle.minutesLost = 999;
                Narrate::obstacleResult(l, w);
                expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
                w.obstacle.cleared = true;
                Narrate::obstacleResult(l, w);
                expectFits(l, kCardLines, kCardCols, kStages[st].nodes[n].name);
            }
        }
        WorldView w = worldAt(st, 8);
        w.state.duelsWon  = 65535;
        w.state.duelsLost = 65535;
        for (bool last : { false, true }) {
            Lines l;
            Narrate::stageClear(l, w, last);
            expectFits(l, kCardLines, kCardCols, kStages[st].name);
        }
    }
    WorldView won = worldAt(0, 1);
    won.duel.outcome      = DuelOutcome::Won;
    won.duel.xp           = 65535;
    won.duel.levelsGained = 3;
    for (ItemId it = 0; it < Item::Count; ++it) {
        won.duel.item = it;
        Lines l;
        Narrate::duelEnd(l, won);
        expectFits(l, kCardLines, kCardCols, itemDef(it).name);
    }
    for (SpeciesId s = 0; s < Spirit::Count; ++s) {
        if (!isBoss(s)) {
            for (bool leaving : { false, true }) {
                Lines l;
                Narrate::offer(l, s, leaving);
                expectFits(l, kCardLines, kCardCols, speciesDef(s).name);
            }
        }
    }
}

TEST(Layout, EveryStoryPageFitsItsCard)
{
    for (StoryId id : { StoryId::Opening, StoryId::StageStart, StoryId::BossAhead, StoryId::BossBeaten }) {
        for (uint8_t st = 0; st < kStageCount; ++st) {
            const Story s = story(id, st);
            ASSERT_GT(s.count, 0u);
            for (uint8_t i = 0; i < s.count; ++i) {
                const StoryPage& p = s.pages[i];
                EXPECT_GT(len(p.head), 0u) << p.body;
                EXPECT_LE(len(p.head), static_cast<size_t>(kHeadCols)) << p.head;
                Lines l;
                Narrate::storyPage(l, p);
                EXPECT_TRUE(l.fits) << p.body;
                EXPECT_LE(l.count, kStoryLines) << p.body;
                for (uint8_t k = 0; k < l.count; ++k) {
                    EXPECT_LE(len(l.text[k]), static_cast<size_t>(kStoryCols)) << p.body << ": " << l.text[k];
                }
                EXPECT_TRUE(p.who == kNone || p.who < Spirit::Count) << p.head;
            }
        }
    }
}

TEST(Layout, EveryWayOnTheMapSaysWhereItLeads)
{
    for (uint8_t st = 0; st < kStageCount; ++st) {
        for (uint8_t n = 0; n < kStages[st].nodeCount; ++n) {
            WorldView w = worldAt(st, n);
            for (uint8_t taken : { uint8_t{0}, uint8_t{0xFF} }) {
                w.state.cachesTaken = taken;
                for (uint8_t e = 0; e < kStages[st].nodes[n].exitCount; ++e) {
                    const char* t = Narrate::leadsTo(w, e);
                    EXPECT_GT(len(t), 0u) << kStages[st].nodes[n].name;
                    EXPECT_LE(len(t), static_cast<size_t>(kLeadsCols)) << t;
                }
                // No such way: nothing to say.
                EXPECT_EQ(len(Narrate::leadsTo(w, kStages[st].nodes[n].exitCount)), 0u);
            }
        }
    }
    // The valley's first fork: the woods have a spirit, the stream a hazard;
    // the old gate's box says if it has been taken.
    WorldView w = worldAt(0, 0);
    EXPECT_STREQ(Narrate::leadsTo(w, 0), "A spirit");
    EXPECT_STREQ(Narrate::leadsTo(w, 1), "A hazard");
    w = worldAt(0, 3);
    EXPECT_STREQ(Narrate::leadsTo(w, 0), "Supplies");
    w.state.cachesTaken = 0x01;
    EXPECT_STREQ(Narrate::leadsTo(w, 0), "Nothing");
    w = worldAt(0, 7);
    EXPECT_STREQ(Narrate::leadsTo(w, 0), "The boss");
}

TEST(Story, APlanPlaysItsStoriesInARow)
{
    const StoryPlan opening = StoryPlan::of(StoryId::Opening, StoryId::StageStart, 0);
    const Story     tale    = story(StoryId::Opening, 0);
    ASSERT_EQ(opening.pageCount(), tale.count + kStageStartPages);
    EXPECT_EQ(opening.page(0), &tale.pages[0]);
    EXPECT_EQ(opening.page(tale.count - 1), &tale.pages[tale.count - 1]);
    EXPECT_EQ(opening.page(tale.count), &kStageStart[0][0]);
    EXPECT_EQ(opening.page(tale.count + 1), &kStageStart[0][1]);
    EXPECT_EQ(opening.page(tale.count + 2), nullptr);
    EXPECT_EQ(opening.page(255), nullptr);

    const StoryPlan boss = StoryPlan::of(StoryId::BossAhead, 2);
    ASSERT_EQ(boss.pageCount(), 1u);
    EXPECT_EQ(boss.page(0), &kBossAhead[2]);
    EXPECT_EQ(boss.page(0)->who, kStages[2].boss);
    EXPECT_EQ(boss.page(1), nullptr);

    // Each stage's goal and its boss pages show its own boss.
    for (uint8_t st = 0; st < kStageCount; ++st) {
        EXPECT_EQ(kStageStart[st][1].who, kStages[st].boss) << int(st);
        EXPECT_EQ(kBossAhead[st].who, kStages[st].boss) << int(st);
        EXPECT_EQ(kBossBeaten[st].who, kStages[st].boss) << int(st);
    }

    // Out of range: the first stage, never past the tables; an empty plan is empty.
    EXPECT_EQ(StoryPlan::of(StoryId::BossBeaten, 200).page(0), &kBossBeaten[0]);
    EXPECT_EQ(StoryPlan{}.pageCount(), 0u);
    EXPECT_EQ(StoryPlan{}.page(0), nullptr);
}

TEST(Layout, TheMapsStaminaBarIsOnTheFace)
{
    // MapScreen: 100 px wide, 6 tall, centred, at kMapBarRow.
    for (int32_t y : { kMapBarRow, kMapBarRow + 6 }) {
        EXPECT_GE(TextFit::chordAt(y), 100 + 2 * TextFit::kMargin) << y;
    }
    EXPECT_TRUE(clearOfArcs(70, kMapBarRow, 170, kMapBarRow + 6));
}
