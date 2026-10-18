/**
 ******************************************************************************
 * @file    DuelScreen.cpp
 * @brief   A duel with a Trail Spirit (see DuelScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/DuelScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/Art.hpp"
#include "gui/screens/ScreenManager.hpp"

#define LOG_MODULE_PRX      "Duel"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

namespace
{
constexpr uint8_t kEventFrames = 9;    ///< Each event of a turn stays this many frames (0.9 s).
constexpr uint8_t kVerdictFrames = 6;  ///< The beat's verdict stays this long before the turn plays.
constexpr uint8_t kFlashFrames = 4;    ///< A hit fighter blinks for the first frames of its event.

// The scene: sky, hills, grass, path; the fighters stand on the path.
constexpr int32_t kRunnerX = 40;
constexpr int32_t kGroundY = 156;      ///< Where feet touch the path.
constexpr int32_t kSpiritRight = 200;  ///< The spirit's right edge.

// Bars: the spirit's at the top, the runner's at the foot.
constexpr int32_t kSpiritBarY = 18;
constexpr int32_t kRunnerBarY = 180;

// The beat replaces the bottom labels while it runs.
constexpr int32_t kBeatX = 30;
constexpr int32_t kBeatY = 164;
constexpr int32_t kBeatW = 180;

const char* actionWord(Action a)
{
    switch (a) {
        case Action::Surge:     return "Surge!";
        case Action::Settle:    return "Settle!";
        case Action::Technique: return "Skill!";
        default:                return "";
    }
}

const char* beatWord(BeatResult r)
{
    return r == BeatResult::Perfect ? "PERFECT" : (r == BeatResult::Good ? "GOOD" : "MISS");
}

uint32_t beatInk(BeatResult r)
{
    return r == BeatResult::Perfect ? Ui::Ink::kGreen : (r == BeatResult::Good ? Ui::Ink::kGold : Ui::Ink::kRed);
}

uint16_t addTo(uint16_t v, uint16_t d, uint16_t max)
{
    const uint32_t s = static_cast<uint32_t>(v) + d;
    return static_cast<uint16_t>(s > max ? max : s);
}

uint16_t takeFrom(uint16_t v, uint16_t d)
{
    return v > d ? static_cast<uint16_t>(v - d) : 0u;
}
} // namespace

void DuelScreen::build()
{
    const DuelView& d = mModel.duel();
    Draw::box(mRoot, 0, 44, 240, 70, Art::skyFor(d.weather, d.dark != 0));
    Draw::box(mRoot, 0, 104, 240, 22, 0x008000);
    Draw::box(mRoot, 0, 122, 240, 18, 0x00C000);
    Draw::box(mRoot, 0, 138, 240, 20, 0x804000);

    mRunner.create(mRoot, kRunnerX, kGroundY - 48, TRAILQUEST_SPRITE_RUNNER, SpriteBuffers::small(0), SpriteBuffers::kSmall);
    const uint8_t sprite = Art::spriteFor(d.species);
    const int32_t side   = trailquest_sprites[sprite].size * Sprite::kScale;
    mSpirit.create(mRoot, kSpiritRight - side, kGroundY - side, sprite, SpriteBuffers::large(), SpriteBuffers::kLarge);

    mSpiritBar.create(mRoot, 70, kSpiritBarY, 100, 6, Ui::Ink::kRed);
    mSpiritNum = Ui::rowLabel(mRoot, Layout::kHeadRow, mSpiritText);
    mRunnerBar.create(mRoot, 45, kRunnerBarY, 150, 6, Ui::Ink::kGreen);
    mRunnerNum = Ui::rowLabel(mRoot, Layout::kFootRow, mRunnerText);
    mNarr[0] = Ui::rowLabel(mRoot, Layout::kNarrRow, mSaid.text[0]);
    mNarr[1] = Ui::rowLabel(mRoot, Layout::kNarrRow + Layout::kNarrPitch, mSaid.text[1]);
    mCorners.create(mRoot);
    mBeat.create(mRoot, kBeatX, kBeatY, kBeatW);
    mCard.create(mRoot);
}

void DuelScreen::onShow()
{
    const DuelView& d = mModel.duel();
    mShownRunner = d.runner.stamina;
    mShownSpirit = d.spirit.stamina;
    mRunnerFx    = d.runner.effect;
    mSpiritFx    = d.spirit.effect;
    refreshNumbers();
    if (d.turn == 0 && d.log.count == 0) {
        TextFit::Lines lines;
        Narrate::spiritIntro(lines, d.species);
        mCard.show(lines, isBoss(d.species) ? Ui::Ink::kRed : Ui::Ink::kGold);
        setMode(Mode::Intro);
    } else {
        setMode(Mode::Choose);
    }
}

void DuelScreen::say(const TextFit::Lines& lines, uint32_t color)
{
    mSaid = lines;
    for (uint8_t i = 0; i < 2; ++i) {
        Ui::setText(mNarr[i], i < mSaid.count ? mSaid.text[i] : "", color);
        Draw::setHidden(mNarr[i], false);
    }
}

void DuelScreen::setMode(Mode m)
{
    mMode = m;
    mBeat.setHidden(m != Mode::Beat);
    if (m != Mode::Intro && m != Mode::End) {
        mCard.hide();
    }
    const DuelView& d = mModel.duel();
    TextFit::Lines  none;
    switch (m) {
        case Mode::Intro:
        case Mode::End:
            say(none, Ui::Ink::kText);
            mCorners.set(nullptr, Label::kOk, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
            break;
        case Mode::Choose: {
            // Row 1 is the corner labels'; row 2 names what R1 plays, in R1's green.
            TextFit::Lines tech;
            TextFit::addLine(tech, "", Layout::kNarrCols);
            TextFit::addLine(tech, moveDef(d.technique).name, Layout::kNarrCols);
            say(tech, Ui::Ink::kGreen);
            mCorners.set(Label::kSurge, Label::kSkill, Label::kSettle, Label::kFuel);
            setArcs(Buttons::AMBER, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
        } break;
        case Mode::Fuel:
            mCorners.set(Label::kNext, Label::kUse, Label::kBack, Label::kBack);
            setArcs(Buttons::AMBER, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
            showFuel();
            break;
        case Mode::Beat: {
            TextFit::Lines prompt;
            TextFit::addLine(prompt, actionWord(mAction), Layout::kNarrCols);
            say(prompt, Ui::Ink::kAmber);
            mCorners.set(nullptr, nullptr, nullptr, nullptr);
            setArcs(Buttons::AMBER, Buttons::AMBER, Buttons::AMBER, Buttons::AMBER);
        } break;
        case Mode::Waiting:
        case Mode::Playback:
            mCorners.set(nullptr, nullptr, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::NONE, Buttons::RED);
            break;
    }
}

void DuelScreen::refreshNumbers()
{
    const DuelView& d = mModel.duel();
    std::snprintf(mSpiritText, sizeof(mSpiritText), "%u/%u %s", static_cast<unsigned>(mShownSpirit),
                  static_cast<unsigned>(d.spirit.max), Narrate::effectTag(mSpiritFx));
    std::snprintf(mRunnerText, sizeof(mRunnerText), "%u/%u %s", static_cast<unsigned>(mShownRunner),
                  static_cast<unsigned>(d.runner.max), Narrate::effectTag(mRunnerFx));
    lv_label_set_text_static(mSpiritNum, mSpiritText);
    lv_label_set_text_static(mRunnerNum, mRunnerText);
    mSpiritBar.set(mShownSpirit, d.spirit.max);
    mRunnerBar.set(mShownRunner, d.runner.max);
}

bool DuelScreen::nextFuel(uint8_t from)
{
    const RunState& s = mModel.world().state;
    for (uint8_t k = 0; k < Item::Count; ++k) {
        const ItemId i = static_cast<ItemId>((from + k) % Item::Count);
        if (!itemDef(i).key && s.bag[i] > 0) {
            mFuelItem = i;
            return true;
        }
    }
    mFuelItem = kNone;
    return false;
}

void DuelScreen::showFuel()
{
    const RunState& s = mModel.world().state;
    std::snprintf(mFuelText, sizeof(mFuelText), "%s x%u", itemDef(mFuelItem).name,
                  static_cast<unsigned>(mFuelItem < Item::Count ? s.bag[mFuelItem] : 0u));
    TextFit::Lines lines;
    TextFit::addLine(lines, "", Layout::kNarrCols);
    TextFit::addLine(lines, mFuelText, Layout::kNarrCols);
    say(lines, Ui::Ink::kText);
}

void DuelScreen::choose(Action a)
{
    mAction = a;
    if (mModel.beatOn() && a != Action::Fuel) {
        mBeat.start(lv_tick_get());
        setMode(Mode::Beat);
    } else {
        send(BeatResult::Good);
    }
}

void DuelScreen::send(BeatResult beat)
{
    if (mModel.beatOn() && mAction != Action::Fuel) {
        TextFit::Lines lines;
        TextFit::addLine(lines, actionWord(mAction), Layout::kNarrCols);
        TextFit::addLine(lines, beatWord(beat), Layout::kNarrCols);
        say(lines, beatInk(beat));
        mHold = kVerdictFrames;
        if (beat == BeatResult::Perfect) {
            mModel.playFx(FxId::Perfect);
        } else if (beat == BeatResult::Miss) {
            mModel.playFx(FxId::Miss);
        }
    }
    setMode(Mode::Waiting);
    mModel.duelTurn(mAction, mAction == Action::Fuel ? mFuelItem : kNone, beat);
}

void DuelScreen::onKey(uint8_t code)
{
    switch (mMode) {
        case Mode::Intro:
            if (code == Btn::R1 || code == Btn::L1 || code == Btn::L2) {
                setMode(Mode::Choose);
            }
            break;
        case Mode::Choose:
            if (code == Btn::L1) {
                choose(Action::Surge);
            } else if (code == Btn::L2) {
                choose(Action::Settle);
            } else if (code == Btn::R1) {
                choose(Action::Technique);
            } else if (code == Btn::R2) {
                if (nextFuel(0)) {
                    setMode(Mode::Fuel);
                } else {
                    TextFit::Lines lines;
                    TextFit::addLine(lines, "", Layout::kNarrCols);   // row 1 is the labels'
                    TextFit::addLine(lines, "Bag empty", Layout::kNarrCols);
                    say(lines, Ui::Ink::kRed);
                }
            }
            break;
        case Mode::Fuel:
            if (code == Btn::L1) {
                nextFuel(static_cast<uint8_t>(mFuelItem + 1u));
                showFuel();
            } else if (code == Btn::R1 && mFuelItem != kNone) {
                choose(Action::Fuel);
            } else if (code == Btn::L2 || code == Btn::R2) {
                setMode(Mode::Choose);
            }
            break;
        case Mode::Beat:
            // Any button's press strikes, judged at the needle on screen.
            if (code == Btn::L1_PRESS || code == Btn::L2_PRESS || code == Btn::R1_PRESS || code == Btn::R2_PRESS) {
                const BeatJudgement j = mBeat.judge();
                LOG_INFO("beat: %s, %ld ms\n", beatWord(j.result), static_cast<long>(j.offsetMs));
                send(j.result);
            }
            break;
        case Mode::End:
            if (code == Btn::R1) {
                ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
            }
            break;
        default:
            break;
    }
}

void DuelScreen::onWorld()
{
    if (mMode != Mode::Waiting || mModel.waiting()) {
        return;
    }
    const WorldView& w = mModel.world();
    if (w.cause != ViewCause::DuelTurn && w.cause != ViewCause::DuelOver) {
        setMode(Mode::Choose);   // refused: nothing happened
        return;
    }
    // Surge's cost is paid before the move (Duel.cpp), and is not an event.
    if (mAction == Action::Surge) {
        const uint8_t cost = moveDef(Move::Surge).selfCost;
        mShownRunner = mShownRunner > cost ? static_cast<uint16_t>(mShownRunner - cost) : 1u;
    }
    mEvent      = 0;
    mEventFrame = 0;
    mStarted    = false;
    setMode(Mode::Playback);   // onFrame() starts it, after the beat's verdict
}

void DuelScreen::showEvent(uint8_t i)
{
    const DuelView&  d = mModel.duel();
    const DuelEvent& e = d.log.events[i < kMaxEvents ? i : 0];
    TextFit::Lines   lines;
    Narrate::event(lines, e, d);

    // Good news for the runner in amber, bad in red.
    bool good = e.actor == Side::Runner;
    if (e.kind == EventKind::Ticked || e.kind == EventKind::Wore) {
        good = (e.kind == EventKind::Wore) == (e.actor == Side::Runner);
    }
    say(lines, good ? Ui::Ink::kAmber : Ui::Ink::kRed);

    mFlash = nullptr;
    switch (e.kind) {
        case EventKind::Hit:
            if (e.actor == Side::Runner) {
                mShownSpirit = takeFrom(mShownSpirit, e.amount);
                mFlash       = mSpirit.object();
            } else {
                mShownRunner = takeFrom(mShownRunner, e.amount);
                mFlash       = mRunner.object();
            }
            mModel.playFx(FxId::Hit);
            break;
        case EventKind::Ticked:
            if (e.actor == Side::Runner) {
                mShownRunner = takeFrom(mShownRunner, e.amount);
            } else {
                mShownSpirit = takeFrom(mShownSpirit, e.amount);
            }
            break;
        case EventKind::Healed:
        case EventKind::UsedItem:
            mShownRunner = addTo(mShownRunner, e.amount, d.runner.max);
            break;
        case EventKind::Rested:
            mShownSpirit = addTo(mShownSpirit, e.amount, d.spirit.max);
            break;
        case EventKind::Inflicted:
            (e.actor == Side::Runner ? mSpiritFx : mRunnerFx) = static_cast<uint8_t>(e.effect);
            break;
        case EventKind::Wore:
            (e.actor == Side::Runner ? mRunnerFx : mSpiritFx) = 0;
            break;
        default:
            break;
    }
    refreshNumbers();
}

void DuelScreen::snapToView()
{
    const DuelView& d = mModel.duel();
    mShownRunner = d.runner.stamina;
    mShownSpirit = d.spirit.stamina;
    mRunnerFx    = d.runner.effect;
    mSpiritFx    = d.spirit.effect;
    Draw::setHidden(mRunner.object(), false);
    Draw::setHidden(mSpirit.object(), false);
    refreshNumbers();
}

void DuelScreen::onFrame(uint32_t nowMs)
{
    if (mMode == Mode::Beat) {
        mBeat.update(nowMs);
        if (mBeat.sweeps(nowMs) >= 2) {
            send(BeatResult::Miss);   // no strike in two passes
        }
        return;
    }
    if (mMode == Mode::Playback) {
        if (mHold > 0) {
            --mHold;
            return;
        }
        if (!mStarted && mModel.duel().log.count > 0) {
            mStarted = true;
            showEvent(0);
            return;
        }
        ++mEventFrame;
        if (mFlash != nullptr) {
            Draw::setHidden(mFlash, mEventFrame <= kFlashFrames && (mEventFrame & 1u) != 0);
        }
        if (mEventFrame < kEventFrames && mModel.duel().log.count > 0) {
            return;
        }
        if (mFlash != nullptr) {
            Draw::setHidden(mFlash, false);
        }
        mEventFrame = 0;
        ++mEvent;
        if (mEvent < mModel.duel().log.count) {
            showEvent(mEvent);
            return;
        }
        snapToView();
        const DuelView& d = mModel.duel();
        if (d.outcome == DuelOutcome::Ongoing) {
            setMode(Mode::Choose);
            return;
        }
        const WorldView& w = mModel.world();
        TextFit::Lines   lines;
        Narrate::duelEnd(lines, w);
        const bool won = w.duel.outcome == DuelOutcome::Won;
        mCard.show(lines, won ? Ui::Ink::kGreen : Ui::Ink::kRed);
        mModel.playFx(won ? (w.duel.levelsGained > 0 ? FxId::LevelUp : FxId::Win) : FxId::Lose);
        setMode(Mode::End);
        return;
    }
    // Idle: the two of them breathe.
    if (nowMs - mLastFlip >= 500) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mRunner.setFrame(mFrame);
        mSpirit.setFrame(mFrame ^ 1u);
    }
}
