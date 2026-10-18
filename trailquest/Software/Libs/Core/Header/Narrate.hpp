/**
 ******************************************************************************
 * @file    Narrate.hpp
 * @brief   Everything the game says, as lines that fit where they are drawn.
 *
 * Header-only: the GUI calls these to fill its labels, and the host tests run
 * them over every spirit, move, item, place and event to prove each block fits
 * its rows (Layout.hpp) on the round face. Short, dry, British English
 * (DESIGN.md, "Voice"). Numbers with %u only (no floating point).
 ******************************************************************************
 */

#ifndef TRAILQUEST_NARRATE_HPP
#define TRAILQUEST_NARRATE_HPP

#include <cstdint>
#include <cstdio>

#include "Layout.hpp"
#include "Progress.hpp"
#include "TextFit.hpp"
#include "View.hpp"
#include "data/Items.hpp"
#include "data/Moves.hpp"
#include "data/Species.hpp"
#include "data/Stages.hpp"
#include "data/Story.hpp"

namespace TrailQuest::Narrate
{

using TextFit::Lines;

/// The words by the buttons' arcs (LayoutTest checks each list fits its
/// corner, inside the arc).
namespace Label
{
constexpr const char* kSurge  = "Surge";    // duel, L1
constexpr const char* kSkill  = "Skill";    // duel, R1: the technique
constexpr const char* kSettle = "Settle";   // duel, L2
constexpr const char* kFuel   = "Fuel";     // duel, R2
constexpr const char* kNext   = "Next";
constexpr const char* kUse    = "Use";
constexpr const char* kBack   = "Back";
constexpr const char* kGo     = "Go";
constexpr const char* kPath   = "Path";
constexpr const char* kPack   = "Pack";
constexpr const char* kYes    = "Yes";
constexpr const char* kNo     = "No";
constexpr const char* kPlay   = "Play";
constexpr const char* kNew    = "New";
constexpr const char* kBeat   = "Beat";
constexpr const char* kOk     = "OK";
constexpr const char* kSkip   = "Skip";

inline constexpr const char* kTopLeft[]     = { kSurge, kNext, kNew, kSkip };       // L1
inline constexpr const char* kTopRight[]    = { kSkill, kUse, kGo, kYes, kPlay, kOk };  // R1
inline constexpr const char* kBottomLeft[]  = { kSettle, kBack, kNo, kPack, kBeat };   // L2
inline constexpr const char* kBottomRight[] = { kFuel, kBack };                        // R2
inline constexpr const char* kMapLeft[]     = { kPath };                               // L1 on the map
inline constexpr const char* kMapRight[]    = { kGo };                                 // R1 on the map
} // namespace Label

inline const char* affinityName(Affinity a)
{
    switch (a) {
        case Affinity::Pace: return "Pace";
        case Affinity::Form: return "Form";
        case Affinity::Grit: return "Grit";
        default:             return "";
    }
}

inline const char* weatherName(uint8_t w)
{
    switch (static_cast<Weather>(w)) {
        case Weather::Clear: return "Clear";
        case Weather::Rain:  return "Rain";
        case Weather::Mist:  return "Mist";
        case Weather::Wind:  return "Wind";
        case Weather::Snow:  return "Snow";
        default:             return "";
    }
}

inline const char* timeName(uint8_t t)
{
    switch (static_cast<TimeOfDay>(t)) {
        case TimeOfDay::Dawn:  return "It's dawn.";
        case TimeOfDay::Day:   return "Broad day.";
        case TimeOfDay::Dusk:  return "Dusk falls.";
        case TimeOfDay::Night: return "It's dark.";
        default:               return "";
    }
}

/// A three-letter tag for an effect, shown by a fighter's numbers.
inline const char* effectTag(uint8_t e)
{
    switch (static_cast<Effect>(e)) {
        case Effect::Drain:  return "DRN";
        case Effect::Slow:   return "SLW";
        case Effect::Rattle: return "RTL";
        default:             return "";
    }
}

/// "07:10" from the trail clock. @p out holds at least 6.
inline void clock(char* out, uint16_t minutes)
{
    const unsigned m = minutes % (24u * 60u);
    std::snprintf(out, 6, "%02u:%02u", m / 60u, m % 60u);
}

/// Two lines (Layout::kNarrCols) for one event of a duel turn.
inline void event(Lines& out, const DuelEvent& e, const DuelView& d)
{
    using Layout::kNarrCols;
    out.clear();
    char num[16];
    const FighterView& target = e.actor == Side::Runner ? d.spirit : d.runner;
    switch (e.kind) {
        case EventKind::Hit:
            TextFit::addLine(out, moveDef(e.move).name, kNarrCols);
            std::snprintf(num, sizeof(num), "-%u", static_cast<unsigned>(e.amount));
            TextFit::addLine(out, num, kNarrCols);
            break;
        case EventKind::Missed:
            TextFit::addLine(out, moveDef(e.move).name, kNarrCols);
            TextFit::addLine(out, "Missed", kNarrCols);
            break;
        case EventKind::Inflicted:
            switch (e.effect) {
                case Effect::Drain:
                    TextFit::addLine(out, "Drained!", kNarrCols);
                    std::snprintf(num, sizeof(num), "-%u a turn",
                                  static_cast<unsigned>(levelScale(kDrainPerTurn, target.level)));
                    break;
                case Effect::Slow:
                    TextFit::addLine(out, "Slowed!", kNarrCols);
                    std::snprintf(num, sizeof(num), "Pace -%u", static_cast<unsigned>(kEffectStatDrop));
                    break;
                default:
                    TextFit::addLine(out, "Rattled!", kNarrCols);
                    std::snprintf(num, sizeof(num), "Grit -%u", static_cast<unsigned>(kEffectStatDrop));
                    break;
            }
            TextFit::addLine(out, num, kNarrCols);
            break;
        case EventKind::Wore:
            TextFit::addLine(out, e.effect == Effect::Drain ? "Drain" : (e.effect == Effect::Slow ? "Slow" : "Rattle"),
                             kNarrCols);
            TextFit::addLine(out, "wears off", kNarrCols);
            break;
        case EventKind::Ticked:
            TextFit::addLine(out, e.effect == Effect::Drain ? "Drained" : "Cold", kNarrCols);
            std::snprintf(num, sizeof(num), "-%u", static_cast<unsigned>(e.amount));
            TextFit::addLine(out, num, kNarrCols);
            break;
        case EventKind::Healed:
            TextFit::addLine(out, "Recover", kNarrCols);
            std::snprintf(num, sizeof(num), "+%u", static_cast<unsigned>(e.amount));
            TextFit::addLine(out, num, kNarrCols);
            break;
        case EventKind::UsedItem: {
            const ItemDef& it = itemDef(e.item);
            TextFit::addLine(out, it.name, kNarrCols);
            if (it.formBoost > 0) {
                std::snprintf(num, sizeof(num), "Form +%u", static_cast<unsigned>(it.formBoost));
            } else {
                std::snprintf(num, sizeof(num), "+%u", static_cast<unsigned>(e.amount));
            }
            TextFit::addLine(out, num, kNarrCols);
        } break;
        case EventKind::NoItem:
            TextFit::addLine(out, "Bag empty", kNarrCols);
            TextFit::addLine(out, "Turn lost", kNarrCols);
            break;
        case EventKind::Rested:
            TextFit::addLine(out, "Second wind!", kNarrCols);
            std::snprintf(num, sizeof(num), "+%u", static_cast<unsigned>(e.amount));
            TextFit::addLine(out, num, kNarrCols);
            break;
    }
}

/// The card when a spirit steps out: its name, then what it does to a runner.
inline void spiritIntro(Lines& out, SpeciesId sp)
{
    out.clear();
    const SpeciesDef& s = speciesDef(sp);
    TextFit::wrap(out, s.name, Layout::kCardTitleCols, 2);
    TextFit::wrap(out, s.pain, Layout::kCardCols, Layout::kCardLines);
    if (out.count > Layout::kCardLines) {
        out.fits = false;
    }
}

/// The card at the end of a duel (WorldView::duel).
inline void duelEnd(Lines& out, const WorldView& w)
{
    using Layout::kCardCols;
    out.clear();
    char line[24];
    const DuelResult& r = w.duel;
    if (r.outcome == DuelOutcome::Won) {
        TextFit::addLine(out, "You won!", kCardCols);
        std::snprintf(line, sizeof(line), "+%u XP", static_cast<unsigned>(r.xp));
        TextFit::addLine(out, line, kCardCols);
        if (r.levelsGained > 0) {
            std::snprintf(line, sizeof(line), "Level %u!", static_cast<unsigned>(w.state.level));
            TextFit::addLine(out, line, kCardCols);
        }
        if (r.item != kNone) {
            std::snprintf(line, sizeof(line), "+1 %s", itemDef(r.item).name);
            TextFit::addLine(out, line, kCardCols);
        }
    } else {
        TextFit::addLine(out, "Spent!", kCardCols);
        std::snprintf(line, sizeof(line), "Back to %s", nodeDef(w.state.stage, w.state.node).name);
        TextFit::wrap(out, line, kCardCols, 2);
        std::snprintf(line, sizeof(line), "+%u min", static_cast<unsigned>(kLossMinutes));
        TextFit::addLine(out, line, kCardCols);
        if (r.lostItem != kNone) {
            std::snprintf(line, sizeof(line), "Lost %s", itemDef(r.lostItem).name);
            TextFit::addLine(out, line, kCardCols);
        }
    }
    if (out.count > Layout::kCardLines) {
        out.fits = false;
    }
}

/// The card on the map: a new stage's conditions (its story has said where
/// it is), a game picked up, or what a step found. Empty (count 0) when there
/// is nothing to say.
inline void arrival(Lines& out, const WorldView& w)
{
    using Layout::kCardCols;
    out.clear();
    char line[32];
    const NodeDef& here = nodeDef(w.state.stage, w.state.node);
    switch (w.cause) {
        case ViewCause::NewGame:
        case ViewCause::StageStarted:
            std::snprintf(line, sizeof(line), "%s today", weatherName(w.state.weather));
            TextFit::addLine(out, line, Layout::kCardTitleCols);
            TextFit::addLine(out, timeName(w.timeOfDay), kCardCols);
            return;
        case ViewCause::Loaded:
            TextFit::addLine(out, "Welcome back", kCardCols);
            TextFit::wrap(out, here.name, kCardCols, 2);
            return;
        case ViewCause::Arrived:
            break;
        default:
            return;
    }
    switch (w.arrive.what) {
        case Arrival::Found:
            TextFit::wrap(out, here.name, Layout::kCardTitleCols, 2);
            if (w.arrive.bagFull) {
                TextFit::addLine(out, "No room for", kCardCols);
                std::snprintf(line, sizeof(line), "%s.", itemDef(w.arrive.item).name);
                TextFit::addLine(out, line, kCardCols);
            } else {
                std::snprintf(line, sizeof(line), "+1 %s", itemDef(w.arrive.item).name);
                TextFit::addLine(out, line, kCardCols);
            }
            break;
        case Arrival::Hut:
            TextFit::wrap(out, here.name, Layout::kCardTitleCols, 2);
            TextFit::addLine(out, "Rested.", kCardCols);
            TextFit::addLine(out, "Stamina full", kCardCols);
            break;
        case Arrival::Nothing:
            TextFit::wrap(out, here.name, Layout::kCardTitleCols, 2);
            TextFit::addLine(out, here.kind == NodeKind::Cache ? "Already empty" : "All quiet.", kCardCols);
            break;
        default:
            break;   // a duel or an obstacle has its own screen
    }
}

/// The obstacle's card before the attempt: its name, what it tests, the odds.
inline void obstacleIntro(Lines& out, const WorldView& w)
{
    using Layout::kCardCols;
    out.clear();
    char line[24];
    const ObstacleDef& ob = obstacleDef(nodeDef(w.state.stage, w.state.node).param);
    TextFit::wrap(out, ob.name, Layout::kCardTitleCols, 2);
    std::snprintf(line, sizeof(line), "Tests %s", affinityName(ob.check));
    TextFit::addLine(out, line, kCardCols);
    std::snprintf(line, sizeof(line), "Good %u%%", static_cast<unsigned>(w.odds[1]));
    TextFit::addLine(out, line, kCardCols);
    std::snprintf(line, sizeof(line), "Perfect %u%%", static_cast<unsigned>(w.odds[2]));
    TextFit::addLine(out, line, kCardCols);
}

/// The obstacle's card after the attempt.
inline void obstacleResult(Lines& out, const WorldView& w)
{
    using Layout::kCardCols;
    out.clear();
    char line[24];
    if (w.obstacle.cleared) {
        TextFit::addLine(out, "Cleared!", kCardCols);
        std::snprintf(line, sizeof(line), "+%u XP", static_cast<unsigned>(kXpObstacle));
        TextFit::addLine(out, line, kCardCols);
    } else {
        TextFit::addLine(out, "Slipped!", kCardCols);
        std::snprintf(line, sizeof(line), "-%u stamina", static_cast<unsigned>(w.obstacle.staminaLost));
        TextFit::addLine(out, line, kCardCols);
        std::snprintf(line, sizeof(line), "+%u min", static_cast<unsigned>(w.obstacle.minutesLost));
        TextFit::addLine(out, line, kCardCols);
    }
}

/// The two lines under a spirit offering to join (or, with a full pack, a
/// partner who might leave).
inline void offer(Lines& out, SpeciesId sp, bool leaving)
{
    out.clear();
    TextFit::addLine(out, speciesDef(sp).name, Layout::kCardCols);
    TextFit::addLine(out, leaving ? "leaves?" : "joins you?", Layout::kCardCols);
}

/// A story page's body, as the story card's lines (its headline is the
/// page's own, shown as it is).
inline void storyPage(Lines& out, const StoryPage& p)
{
    out.clear();
    TextFit::wrap(out, p.body, Layout::kStoryCols, Layout::kStoryLines);
}

/// What lies along exit @p exit from where the runner stands, for the map's
/// row under the way's name (Layout::kLeadsCols): every place says what it is
/// before the runner goes there.
inline const char* leadsTo(const WorldView& w, uint8_t exit)
{
    const NodeDef& here = nodeDef(w.state.stage, w.state.node);
    if (exit >= here.exitCount || exit >= kMaxExits) {
        return "";
    }
    const uint8_t  bit = static_cast<uint8_t>(1u << (w.state.stage < 8 ? w.state.stage : 7));
    const NodeDef& to  = nodeDef(w.state.stage, here.exits[exit].to);
    switch (to.kind) {
        case NodeKind::Encounter: return "A spirit";
        case NodeKind::Obstacle:  return "A hazard";
        case NodeKind::Hut:       return "A hut";
        case NodeKind::Cache:     return (w.state.cachesTaken & bit) != 0 ? "Nothing" : "Supplies";
        case NodeKind::Boss:      return "The boss";
        case NodeKind::Start:     return "The start";
        case NodeKind::Trail:
        default:                  return "Quiet";
    }
}

/// The card when a stage's boss is beaten. @p last is true when this build
/// goes no further (the playable stages end here).
inline void stageClear(Lines& out, const WorldView& w, bool last)
{
    using Layout::kCardCols;
    out.clear();
    char line[24];
    TextFit::wrap(out, stageDef(w.state.stage).name, Layout::kCardTitleCols, 2);
    TextFit::addLine(out, "cleared!", kCardCols);
    if (last) {
        TextFit::addLine(out, "More trail", kCardCols);
        TextFit::addLine(out, "coming soon", kCardCols);
    } else {
        std::snprintf(line, sizeof(line), "Won %u", static_cast<unsigned>(w.state.duelsWon));
        TextFit::addLine(out, line, kCardCols);
    }
}

} // namespace TrailQuest::Narrate

#endif // TRAILQUEST_NARRATE_HPP
