/**
 ******************************************************************************
 * @file    ObstacleScreen.cpp
 * @brief   An obstacle on the trail (see ObstacleScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/ObstacleScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/screens/ScreenManager.hpp"

#define LOG_MODULE_PRX      "Obstacle"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

namespace
{
/// The beat's track: under the card, where the bottom labels would be.
constexpr int32_t kBeatX = 30;
constexpr int32_t kBeatY = 164;
constexpr int32_t kBeatW = 180;

const char* beatWord(BeatResult r)
{
    return r == BeatResult::Perfect ? "PERFECT" : (r == BeatResult::Good ? "GOOD" : "MISS");
}

uint32_t beatInk(BeatResult r)
{
    return r == BeatResult::Perfect ? Ui::Ink::kGreen : (r == BeatResult::Good ? Ui::Ink::kGold : Ui::Ink::kRed);
}
} // namespace

void ObstacleScreen::build()
{
    // A grey crag behind the card, so this reads as rock, not a menu.
    Draw::box(mRoot, 0, 50, 240, 120, 0x404040);
    Draw::box(mRoot, 0, 140, 240, 30, 0x808080);
    mHead = Ui::rowLabel(mRoot, Layout::kHeadRow, "", Ui::Ink::kGold);
    mCard.create(mRoot);
    mCorners.create(mRoot);
    mBeat.create(mRoot, kBeatX, kBeatY, kBeatW, beatFor(mModel.world().state.stage, mModel.world().state.lap));
    mBeat.setHidden(true);
}

void ObstacleScreen::onShow()
{
    TextFit::Lines lines;
    Narrate::obstacleIntro(lines, mModel.world());
    mCard.show(lines);
    setMode(Mode::Intro);
}

void ObstacleScreen::setMode(Mode m)
{
    mMode = m;
    mBeat.setHidden(m != Mode::Beat);
    switch (m) {
        case Mode::Intro:
            Ui::setText(mHead, "OBSTACLE", Ui::Ink::kGold);
            mCorners.set(nullptr, Label::kGo, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
            break;
        case Mode::Beat:
            Ui::setText(mHead, "NOW!", Ui::Ink::kAmber);
            mCorners.set(nullptr, nullptr, nullptr, nullptr);
            setArcs(Buttons::AMBER, Buttons::AMBER, Buttons::AMBER, Buttons::AMBER);
            break;
        case Mode::Waiting:
            mCorners.set(nullptr, nullptr, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::NONE, Buttons::RED);
            break;
        case Mode::Result:
            mCorners.set(nullptr, Label::kOk, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::GREEN, Buttons::RED);
            break;
    }
}

void ObstacleScreen::attempt(BeatResult beat)
{
    if (mModel.beatOn()) {
        Ui::setText(mHead, beatWord(beat), beatInk(beat));
        if (beat == BeatResult::Perfect) {
            mModel.playFx(FxId::Perfect);
        } else if (beat == BeatResult::Miss) {
            mModel.playFx(FxId::Miss);
        }
    }
    setMode(Mode::Waiting);
    mModel.obstacleTry(beat);
}

void ObstacleScreen::onKey(uint8_t code)
{
    switch (mMode) {
        case Mode::Intro:
            if (code == Btn::R1) {
                if (mModel.beatOn()) {
                    mBeat.start(lv_tick_get());
                    setMode(Mode::Beat);
                } else {
                    attempt(BeatResult::Good);
                }
            }
            break;
        case Mode::Beat:
            // Any button's press strikes, judged at the needle on screen.
            if (code == Btn::L1_PRESS || code == Btn::L2_PRESS || code == Btn::R1_PRESS || code == Btn::R2_PRESS) {
                const BeatJudgement j = mBeat.judge();
                LOG_INFO("beat: %s, %ld ms\n", beatWord(j.result), static_cast<long>(j.offsetMs));
                attempt(j.result);
            }
            break;
        case Mode::Result:
            if (code == Btn::R1) {
                ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
            }
            break;
        default:
            break;
    }
}

void ObstacleScreen::onFrame(uint32_t nowMs)
{
    if (mMode == Mode::Beat) {
        mBeat.update(nowMs);
        if (mBeat.sweeps(nowMs) >= 2) {
            attempt(BeatResult::Miss);   // no strike in two passes
        }
    }
}

void ObstacleScreen::onWorld()
{
    if (mMode != Mode::Waiting || mModel.waiting()) {
        return;
    }
    const WorldView& w = mModel.world();
    if (mModel.lost() || w.cause != ViewCause::ObstacleDone) {
        setMode(Mode::Intro);   // refused: try again
        return;
    }
    TextFit::Lines lines;
    Narrate::obstacleResult(lines, w);
    mCard.show(lines, w.obstacle.cleared ? Ui::Ink::kGreen : Ui::Ink::kRed);
    mModel.playFx(w.obstacle.cleared ? FxId::Hit : FxId::Miss);
    setMode(Mode::Result);
}
