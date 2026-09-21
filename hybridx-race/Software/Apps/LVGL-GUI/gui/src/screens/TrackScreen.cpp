/**
 ******************************************************************************
 * @file    TrackScreen.cpp
 * @brief   The race screen (see TrackScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackScreen.hpp"

#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RaceModel.hpp"

using namespace SDK::GUI;

TrackScreen::TrackScreen(Model& model)
    : Screen(model)
{
}

void TrackScreen::build()
{
    using F = Theme::Font;

    mMainRoot = Theme::container(mRoot, 0, 0, 240, 240);
    // Segment identity: what am I doing, and how far through am I.
    mSegment     = Theme::label(mMainRoot, F::Italic18, Strings::kNoValue, 0, 44, 240);
    mSegmentNum  = Theme::label(mMainRoot, F::Medium18, "", 0, 66, 240);
    // The number the athlete actually looks at mid-effort, in the largest face.
    mSegmentTime = Theme::label(mMainRoot, F::SemiBold40, "0:00", 0, 92, 240);
    mTotalTime   = Theme::label(mMainRoot, F::Medium18, "0:00:00", 0, 136, 240);
    mNextUp      = Theme::label(mMainRoot, F::Italic18, "", 0, 198, 240);
    mHr          = Theme::label(mMainRoot, F::Medium18, Strings::kNoValue, 0, 162, 240);
    mHrZone      = std::make_unique<Widgets::HeartRateZone>(mMainRoot, 95, 182);

    mStatusRoot = Theme::container(mRoot, 0, 0, 240, 240);
    mClock      = Theme::label(mStatusRoot, F::SemiBold40, "--:--", 0, 90, 240);
    mBattery    = Theme::label(mStatusRoot, F::Medium18, "--%", 0, 140, 240);

    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Race");
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::WHITE, Widgets::Buttons::AMBER);

    showFace(Face::Main);
}

void TrackScreen::onShow()
{
    // No resetIdleTimer(): this screen has no idle timeout at all, and the
    // model's timer is irrelevant here.
    redraw();
}

void TrackScreen::onHide()
{
}

void TrackScreen::showFace(Face face)
{
    mFace = face;
    const bool main = (face == Face::Main);
    if (main) {
        lv_obj_remove_flag(mMainRoot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mStatusRoot, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mMainRoot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(mStatusRoot, LV_OBJ_FLAG_HIDDEN);
    }
}

void TrackScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
    case Btn::L1:
        showFace(mFace == Face::Main ? Face::Status : Face::Main);
        break;

    case Btn::L2:
        showFace(mFace == Face::Main ? Face::Status : Face::Main);
        break;

    case Btn::R1:
        ScreenManager::instance().goTo(ScreenId::RaceAction);
        break;

    case Btn::R2:
        // The split is stamped in the model at this instant (brief 7.4). The
        // service decides whether the lockout swallows it; if it does, no
        // SPLIT_EVENT comes back and nothing on screen changes, which is
        // exactly invariant 3.
        mModel.raceSplit();
        break;

    default:
        break;
    }
}

void TrackScreen::redraw()
{
    const Track::Data& d = mModel.getRaceData();
    char buf[Race::kMaxLabelLen];

    Race::RaceModel::label(d.current, buf, sizeof(buf));
    lv_label_set_text(mSegment, buf);

    snprintf(buf, sizeof(buf), "%u of %u", static_cast<unsigned>(d.segmentIndex + 1u),
             static_cast<unsigned>(d.segmentCount));
    lv_label_set_text(mSegmentNum, buf);

    Fmt::shortTime(buf, sizeof(buf), Fmt::msToSec(d.segmentMs));
    lv_label_set_text(mSegmentTime, buf);

    Fmt::hms(buf, sizeof(buf), Fmt::msToSec(d.totalMs));
    lv_label_set_text(mTotalTime, buf);

    if (d.hasNext) {
        char next[Race::kMaxLabelLen];
        Race::RaceModel::label(d.next, next, sizeof(next));
        snprintf(buf, sizeof(buf), "Next: %s", next);
        lv_label_set_text(mNextUp, buf);
    } else {
        lv_label_set_text(mNextUp, "Last segment");
    }

    // The live readout is ungated on purpose (brief 14.12): the trust gate is
    // for what goes into the FIT file, not for what the athlete sees.
    if (d.hr >= App::Display::kMinHR) {
        snprintf(buf, sizeof(buf), "%u bpm", static_cast<unsigned>(d.hr));
        lv_label_set_text(mHr, buf);
    } else {
        lv_label_set_text(mHr, Strings::kNoValue);
    }
    mHrZone->setHR(static_cast<float>(d.hr), mModel.getHrThresholds(),
                   mModel.getHrThresholdsCount());
}

void TrackScreen::onRaceData(const Track::Data& /*data*/)
{
    redraw();
}

void TrackScreen::onRaceState(const Track::State& /*state*/)
{
    redraw();
}

void TrackScreen::onSplit(const Track::SplitEvent& split)
{
    // The toast confirms the segment that just ended; the finish has its own
    // screen, so do not flash a toast on the way there.
    if (!split.raceFinished) {
        ScreenManager::instance().goTo(ScreenId::RaceSplit);
    }
}

void TrackScreen::onRaceFinished(bool /*completed*/)
{
    ScreenManager::instance().goTo(ScreenId::RaceFinished);
}

void TrackScreen::onTime(uint8_t hour, uint8_t minute, uint8_t /*sec*/)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%02u:%02u", static_cast<unsigned>(hour),
             static_cast<unsigned>(minute));
    lv_label_set_text(mClock, buf);
}

void TrackScreen::onBatteryLevel(uint8_t level)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u%%", static_cast<unsigned>(level));
    lv_label_set_text(mBattery, buf);
}
