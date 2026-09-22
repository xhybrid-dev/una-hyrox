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

namespace
{
/// Accent per segment type (brief 8.2 item 3). The display is 2 bits per
/// channel, so these are picked for separation at arm's length rather than
/// subtlety: cool for running, bright for work, magenta for the transition.
lv_color_t accentFor(Race::SegmentType type)
{
    switch (type) {
    case Race::SegmentType::Station: return Theme::rgb(Color::LEMON);
    case Race::SegmentType::RoxIn:
    case Race::SegmentType::RoxOut:  return Theme::rgb(Color::ORCHID);
    case Race::SegmentType::Run:
    default:                         return Theme::rgb(Color::CYAN);
    }
}

}  // namespace

void TrackScreen::build()
{
    using F = Theme::Font;

    mMainRoot = Theme::container(mRoot, 0, 0, 240, 240);

    // Identity: what am I doing, and how far through.
    mSegment    = Theme::label(mMainRoot, F::Italic18, Strings::kNoValue, 0, 42, 240);
    mSegmentNum = Theme::label(mMainRoot, F::Medium18, "", 0, 64, 240,
                               LV_TEXT_ALIGN_CENTER, Color::GRAY);

    // The number the athlete reads mid-effort, in the largest face that still
    // leaves room for the four lines brief 8.2 puts under it.
    mSegmentTime = Theme::label(mMainRoot, F::SemiBold40, "0:00", 0, 80, 240);

    mTotalTime = Theme::label(mMainRoot, F::Medium18, "0:00:00", 0, 124, 240,
                              LV_TEXT_ALIGN_CENTER, Color::WHITE);

    // Brief 8.2 calls this the "small" line, and small is what buys the heart
    // rate the clearance it needs above the arc.
    mNextUp = Theme::label(mMainRoot, F::Regular14, "", 0, 148, 240,
                           LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mHr     = Theme::label(mMainRoot, F::Medium18, Strings::kNoValue, 0, 160, 240);

    // The zone bar is 210 x 69, so x = 15 centres it. Its arc is the bottom of
    // a circle centred at (120, 302) with radius 113, so its topmost pixel is
    // y = 185: anything drawn below that is painted over. The first layout put
    // the heart rate at y = 166 and the arc swallowed the bottom of the text.
    mHrZone = std::make_unique<Widgets::HeartRateZone>(mMainRoot, 15, 186);

    mStatusRoot = Theme::container(mRoot, 0, 0, 240, 240);
    mClock      = Theme::label(mStatusRoot, F::SemiBold40, "--:--", 0, 84, 240);
    mBatteryPct = Theme::label(mStatusRoot, F::Medium18, "--%", 0, 136, 240,
                               LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mBattery    = std::make_unique<Widgets::Battery>(mStatusRoot, 75, 166);

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

    // Brief 7.2's one-line form ("BURPEE BROAD JUMPS \xC2\xB7 80 m") is 25
    // characters and runs off both sides of a round 240 px display, so the race
    // face splits it: the name carries the accent, the work joins the counter on
    // the grey line below. RaceModel::label() is unchanged and still what the
    // summary, the toast and the FIT lap names use.
    Race::RaceModel::name(d.current, buf, sizeof(buf));
    lv_label_set_text(mSegment, buf);
    lv_obj_set_style_text_color(mSegment, accentFor(d.current.type), LV_PART_MAIN);

    // A paused clock must never be mistaken for a slow one.
    lv_obj_set_style_text_color(mSegmentTime,
                                mModel.isRacePaused() ? Theme::rgb(Color::GRAY)
                                                      : Theme::rgb(Color::WHITE),
                                LV_PART_MAIN);

    const char *work = Race::RaceModel::work(d.current, mModel.getSettings().runDistanceM);
    if (work[0] != '\0') {
        snprintf(buf, sizeof(buf), "%s %s %u of %u", work, Race::kLabelSep,
                 static_cast<unsigned>(d.segmentIndex + 1u),
                 static_cast<unsigned>(d.segmentCount));
    } else {
        snprintf(buf, sizeof(buf), "%u of %u",
                 static_cast<unsigned>(d.segmentIndex + 1u),
                 static_cast<unsigned>(d.segmentCount));
    }
    lv_label_set_text(mSegmentNum, buf);

    Fmt::shortTime(buf, sizeof(buf), Fmt::msToSec(d.segmentMs));
    lv_label_set_text(mSegmentTime, buf);

    Fmt::hms(buf, sizeof(buf), Fmt::msToSec(d.totalMs));
    lv_label_set_text(mTotalTime, buf);

    if (d.hasNext) {
        // Name only: brief 8.2 writes this as "Next: Sled Pull", and the work as
        // well would run past the edge of the display.
        char name[Race::kMaxNameLen];
        Race::RaceModel::name(d.next, name, sizeof(name));
        snprintf(buf, sizeof(buf), "Next: %s", name);
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
    lv_label_set_text(mBatteryPct, buf);
    mBattery->setLevel(level);
}
