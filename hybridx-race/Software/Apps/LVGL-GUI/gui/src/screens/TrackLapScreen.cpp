/**
 ******************************************************************************
 * @file    TrackLapScreen.cpp
 * @brief   Split toast (see TrackLapScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackLapScreen.hpp"

#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RaceModel.hpp"

namespace
{
/// Brief 8.2: "shows for about 2 s".
constexpr uint32_t kDismissMs = 2000;
}  // namespace

TrackLapScreen::TrackLapScreen(Model& model)
    : Screen(model)
{
}

TrackLapScreen::~TrackLapScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackLapScreen::build()
{
    using F = Theme::Font;
    mSegment = Theme::label(mRoot, F::Italic18, Strings::kNoValue, 0, 86, 240);
    mTime    = Theme::label(mRoot, F::SemiBold40, "0:00", 0, 116, 240);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Split");
}

void TrackLapScreen::onShow()
{
    mModel.resetIdleTimer();

    const Track::SplitEvent& s = mModel.getLastSplit();
    char buf[Race::kMaxLabelLen];

    // The segment that just ENDED, not the one now open: that is what the
    // athlete wants confirmed. Brief 8.2 item 4 writes this toast as
    // "SkiErg 4:12" -- name and time, no work -- and the name alone is also the
    // only form that fits the width at a glance.
    Race::RaceModel::name(s.desc, buf, sizeof(buf));
    lv_label_set_text(mSegment, buf);

    Fmt::shortTime(buf, sizeof(buf),
                   static_cast<std::time_t>(s.activeMs / 1000u));
    lv_label_set_text(mTime, buf);

    mDismiss = lv_timer_create(&TrackLapScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackLapScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    // Any click dismisses early; R2 in particular must not be swallowed.
    if (code == Btn::L1 || code == Btn::L2 || code == Btn::R1 || code == Btn::R2) {
        backToRace();
    }
}

void TrackLapScreen::backToRace()
{
    ScreenManager::instance().goTo(ScreenId::Race);
}

void TrackLapScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackLapScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;  // one-shot: LVGL deletes the timer after this call
    self->backToRace();
}
