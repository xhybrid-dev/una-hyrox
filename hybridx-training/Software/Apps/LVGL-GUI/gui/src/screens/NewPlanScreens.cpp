/**
 ******************************************************************************
 * @file    NewPlanScreens.cpp
 * @brief   New plan and the phone's ask (see the header).
 ******************************************************************************
 */

#include "gui/screens/NewPlanScreens.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"
#include "gui/Assets.hpp"

#include "Commands.hpp"
#include "Date.hpp"       // header-only
#include "TimeText.hpp"

#include <cstdio>
#include <cstring>

namespace Color = SDK::GUI::Color;
using F     = Theme::Font;
using Style = WheelMenu::Item::Style;
using Info  = CustomMessage::PlanSetupInfo;
using Act   = CustomMessage::NewPlan;

namespace
{
const char* const kDayNames[7] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };

const char* levelWords(uint8_t level)
{
    switch (level) {
        case 1:  return "1: up to 20 km/week";
        case 3:  return "3: over 40 km/week";
        default: return "2: 20 to 40 km/week";
    }
}

uint8_t bitCount(uint8_t m)
{
    uint8_t n = 0;
    for (; m != 0; m = static_cast<uint8_t>(m & (m - 1))) {
        ++n;
    }
    return n;
}

/// The long run on a run day: kept if it is one, else the last run day of the week.
void fixLongDay(Info& d)
{
    if (((d.days >> d.longDay) & 1u) != 0 || d.days == 0) {
        return;
    }
    for (int k = 6; k >= 0; --k) {
        if (((d.days >> k) & 1u) != 0) {
            d.longDay = static_cast<uint8_t>(k);
            return;
        }
    }
}
} // namespace

namespace NewPlanText
{
const char* planName(uint8_t plan)
{
    switch (plan) {
        case 1:  return "10K";
        case 2:  return "Half marathon";
        case 3:  return "VO2max block";
        default: return "";
    }
}

void days(uint8_t mask, char* buf, size_t cap)
{
    size_t len = 0;
    buf[0] = '\0';
    for (unsigned d = 0; d < 7; ++d) {
        if (((mask >> d) & 1u) != 0 && len + 5 < cap) {
            len += static_cast<size_t>(snprintf(buf + len, cap - len, "%s%s", len ? " " : "",
                                                XTraining::TimeText::weekdayName(d)));
        }
    }
}
} // namespace NewPlanText

// -- New plan's choices ------------------------------------------------------------------

uint16_t NewPlanScreen::sPos = NewPlanScreen::Menu::ID_DEFAULT;

NewPlanScreen::NewPlanScreen(Model& model) : Screen(model) {}
NewPlanScreen::~NewPlanScreen() = default;

void NewPlanScreen::build()
{
    mItems[Menu::ID_PLAN]   = { Style::Tip, "Plan" };
    mItems[Menu::ID_FIVE_K] = { Style::Tip, "Your 5K" };
    mItems[Menu::ID_RECENT] = { Style::Toggle, "Run in the\nlast 6 weeks", "Recent 5K", &poppins_semibold_25 };
    mItems[Menu::ID_LEVEL]  = { Style::Tip, "Level" };
    mItems[Menu::ID_DAYS]   = { Style::Tip, "Run days" };
    mItems[Menu::ID_LONG]   = { Style::Tip, "Long run" };
    mItems[Menu::ID_RACE]   = { Style::Tip, "Race" };
    mItems[Menu::ID_NEXT]   = { Style::Simple, "Next", nullptr, &poppins_semibold_35 };
    for (uint16_t i = 0; i < Menu::ID_COUNT; ++i) {
        if (mItems[i].style == Style::Tip) {
            mItems[i].tip     = mTip[i];
            mItems[i].itemTip = "";
        }
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "NEW PLAN");
}

void NewPlanScreen::onShow()
{
    mModel.resetIdleTimer();
    mMenu->select(sPos);
    if (mModel.draftReady()) {
        refresh();
    } else {
        mModel.requestPlan(CustomMessage::PlanRequest::Setup);   // seeds the draft (onPlanSetup)
    }
}

void NewPlanScreen::onHide()
{
    sPos = mMenu->selected();
}

void NewPlanScreen::onPlanSetup()
{
    if (mModel.draftReady()) {
        return;
    }
    // From the phone's set-up, with sensible values where it has none.
    Info d = mModel.getPhoneSetup();
    if (d.plan < 1 || d.plan > 3) {
        d.plan = 1;
    }
    if (d.level < 1 || d.level > 3) {
        d.level = 2;
    }
    if (d.fiveKSec == 0) {
        d.fiveKSec = 25 * 60;
    }
    if (d.days == 0) {
        d.days    = 0x56;   // Tue, Wed, Fri, Sun (the phone's defaults)
        d.longDay = 6;
    }
    fixLongDay(d);
    mModel.draft() = d;
    mModel.setDraftReady(true);
    refresh();
}

void NewPlanScreen::refresh()
{
    const Info& d = mModel.draft();
    snprintf(mTip[Menu::ID_PLAN], sizeof(mTip[0]), "%s", NewPlanText::planName(d.plan));
    snprintf(mTip[Menu::ID_FIVE_K], sizeof(mTip[0]), "%u:%02u", static_cast<unsigned>(d.fiveKSec / 60),
             static_cast<unsigned>(d.fiveKSec % 60));
    mItems[Menu::ID_RECENT].toggleState = d.fiveKRecent;
    snprintf(mTip[Menu::ID_LEVEL], sizeof(mTip[0]), "%s", levelWords(d.level));
    NewPlanText::days(d.days, mTip[Menu::ID_DAYS], sizeof(mTip[0]));
    snprintf(mTip[Menu::ID_LONG], sizeof(mTip[0]), "%s", kDayNames[d.longDay < 7 ? d.longDay : 6]);
    if (d.raceDate != 0) {
        XTraining::TimeText::dayDate(d.raceDate, mTip[Menu::ID_RACE], sizeof(mTip[0]), true);
    } else {
        snprintf(mTip[Menu::ID_RACE], sizeof(mTip[0]), "None");
    }
    for (auto& t : mTip) {
        WorkoutUi::fitWidth(t, sizeof(t), Theme::font(F::Italic18), 180);
    }
    mMenu->refresh();
}

void NewPlanScreen::confirm()
{
    if (!mModel.draftReady()) {
        return;
    }
    Info& d = mModel.draft();
    switch (mMenu->selected()) {
        case Menu::ID_PLAN:   d.plan = static_cast<uint8_t>(d.plan % 3 + 1); break;     // 10K, half, VO2max
        case Menu::ID_RECENT: d.fiveKRecent = !d.fiveKRecent; break;
        case Menu::ID_LEVEL:  d.level = static_cast<uint8_t>(d.level % 3 + 1); break;
        case Menu::ID_LONG:
            // The next run day round the week.
            for (int k = 1; k <= 7; ++k) {
                const uint8_t day = static_cast<uint8_t>((d.longDay + k) % 7);
                if (((d.days >> day) & 1u) != 0) {
                    d.longDay = day;
                    break;
                }
            }
            break;
        case Menu::ID_FIVE_K: ScreenManager::instance().goTo(ScreenId::NewPlanFiveK); return;
        case Menu::ID_DAYS:   ScreenManager::instance().goTo(ScreenId::NewPlanDays); return;
        case Menu::ID_RACE:   ScreenManager::instance().goTo(ScreenId::NewPlanRace); return;
        case Menu::ID_NEXT:   ScreenManager::instance().goTo(ScreenId::NewPlanConfirm); return;
        default: break;
    }
    refresh();
}

void NewPlanScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2:
            // Out: the draft is dropped.
            mModel.setDraftReady(false);
            sPos = Menu::ID_DEFAULT;
            ScreenManager::instance().goTo(ScreenId::PlanOverview);
            break;
        default: break;
    }
}

// -- The 5K ------------------------------------------------------------------------------

FiveKPickerScreen::FiveKPickerScreen(Model& model) : Screen(model) {}
FiveKPickerScreen::~FiveKPickerScreen() = default;

void FiveKPickerScreen::build()
{
    mPicker = std::make_unique<Widgets::TwoTonePicker>(mRoot);
    mPicker->setTitle("YOUR 5K");
}

void FiveKPickerScreen::onShow()
{
    mModel.resetIdleTimer();
    mTime.seed(mModel.draft().fiveKSec);
    mTime.render(*mPicker);
}

void FiveKPickerScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mTime.dec(); mTime.render(*mPicker); break;
        case Btn::L2: mTime.inc(); mTime.render(*mPicker); break;
        case Btn::R1:
            if (!mTime.atSec()) {
                mTime.toSec();
                mTime.render(*mPicker);
            } else {
                // 12:00 to 59:59, as the phone's field.
                uint32_t s = mTime.totalSeconds();
                s = s < 12 * 60 ? 12 * 60 : s;
                mModel.draft().fiveKSec = s;
                ScreenManager::instance().goTo(ScreenId::NewPlan);
            }
            break;
        case Btn::R2:
            if (mTime.atSec()) {
                mTime.toMin();
                mTime.render(*mPicker);
            } else {
                ScreenManager::instance().goTo(ScreenId::NewPlan);
            }
            break;
        default: break;
    }
}

// -- The run days ------------------------------------------------------------------------

RunDaysScreen::RunDaysScreen(Model& model) : Screen(model) {}
RunDaysScreen::~RunDaysScreen() = default;

void RunDaysScreen::build()
{
    for (uint8_t d = 0; d < 7; ++d) {
        mItems[d] = { Style::Toggle, kDayNames[d], nullptr, &poppins_semibold_25 };
    }
    mItems[7] = { Style::Simple, "Done", nullptr, &poppins_semibold_35 };
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, 8);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mTitleText);
}

void RunDaysScreen::onShow()
{
    mModel.resetIdleTimer();
    refresh();
}

void RunDaysScreen::refresh()
{
    const uint8_t days = mModel.draft().days;
    for (uint8_t d = 0; d < 7; ++d) {
        mItems[d].toggleState = ((days >> d) & 1u) != 0;
    }
    snprintf(mTitleText, sizeof(mTitleText), "%u RUN DAYS", static_cast<unsigned>(bitCount(days)));
    mTitle->setText(mTitleText);
    mMenu->refresh();
}

void RunDaysScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (mMenu->selected() < 7) {
                Info& d = mModel.draft();
                d.days = static_cast<uint8_t>(d.days ^ (1u << mMenu->selected()));
                fixLongDay(d);
                refresh();
                break;
            }
            ScreenManager::instance().goTo(ScreenId::NewPlan);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::NewPlan);
            break;
        default: break;
    }
}

// -- The race date -----------------------------------------------------------------------

RaceDateScreen::RaceDateScreen(Model& model) : Screen(model) {}
RaceDateScreen::~RaceDateScreen() = default;

void RaceDateScreen::build()
{
    // None, then the weekends from three weeks on (Today's date, from the service).
    using namespace XTraining;
    mCount = 0;
    mDate[mCount] = 0;
    snprintf(mText[mCount], sizeof(mText[0]), "None");
    ++mCount;
    const uint32_t today = mModel.todayDate();
    if (Date::valid(today)) {
        const DayNum t = Date::toDayNum(today);
        const DayNum saturday = Date::mondayOf(t) + 5;
        for (uint16_t w = kFirstWeek; w <= kLastWeek && mCount + 1 < kMaxItems; ++w) {
            for (int k = 0; k < 2; ++k) {
                const Ymd d = Date::fromDayNum(saturday + static_cast<DayNum>(w) * 7 + k);
                mDate[mCount] = d;
                TimeText::dayDate(d, mText[mCount], sizeof(mText[0]));
                ++mCount;
            }
        }
    }
    mItems = std::make_unique<WheelMenu::Item[]>(mCount);
    for (uint16_t i = 0; i < mCount; ++i) {
        mItems[i] = { Style::Simple, mText[i], nullptr, &poppins_semibold_30 };
    }
    mMenu = std::make_unique<WheelMenu>(mRoot, mItems.get(), mCount);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "RACE DATE");
}

void RaceDateScreen::onShow()
{
    mModel.resetIdleTimer();
    uint16_t sel = 0;
    for (uint16_t i = 0; i < mCount; ++i) {
        if (mDate[i] == mModel.draft().raceDate) {
            sel = i;
        }
    }
    mMenu->select(sel);
}

void RaceDateScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            mModel.draft().raceDate = mDate[mMenu->selected() < mCount ? mMenu->selected() : 0];
            ScreenManager::instance().goTo(ScreenId::NewPlan);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::NewPlan);
            break;
        default: break;
    }
}

// -- The new plan, to start --------------------------------------------------------------

PlanConfirmScreen::PlanConfirmScreen(Model& model) : Screen(model) {}
PlanConfirmScreen::~PlanConfirmScreen() = default;

void PlanConfirmScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "START IT?");
    mStack.init(mRoot);
}

void PlanConfirmScreen::onShow()
{
    mModel.resetIdleTimer();
    fill();
    mModel.newPlan(Act::Preview, mModel.draft());
}

void PlanConfirmScreen::onPlanPreview()
{
    fill();
}

void PlanConfirmScreen::fill()
{
    using Line = TextStack::Line;
    const Info& d = mModel.draft();
    const CustomMessage::PlanPreviewInfo& p = mModel.getPlanPreview();
    for (auto& t : mText) {
        t[0] = '\0';
    }
    Line lines[TextStack::kMaxLines];
    uint8_t n = 0;
    snprintf(mText[0], sizeof(mText[0]), "%s", NewPlanText::planName(d.plan));
    lines[n++] = Line{ mText[0], F::SemiBold25, F::SemiBold20, Color::WHITE };
    snprintf(mText[1], sizeof(mText[1]), "Level %u, 5K %u:%02u%s", static_cast<unsigned>(d.level),
             static_cast<unsigned>(d.fiveKSec / 60), static_cast<unsigned>(d.fiveKSec % 60),
             d.fiveKRecent ? "" : " (old)");
    lines[n++] = Line{ mText[1], F::Medium18, F::Regular16, Color::WHITE };
    NewPlanText::days(d.days, mText[2], sizeof(mText[2]));
    lines[n++] = Line{ mText[2], F::Medium18, F::Regular16, Color::WHITE };
    snprintf(mText[3], sizeof(mText[3]), "Long run %s", XTraining::TimeText::weekdayName(d.longDay));
    lines[n++] = Line{ mText[3], F::Italic18, F::Italic18, Color::GRAY };
    if (p.ok) {
        char day[20];
        XTraining::TimeText::dayDate(p.start, day, sizeof(day));
        snprintf(mText[4], sizeof(mText[4]), "From %s", day);
        lines[n++] = Line{ mText[4], F::Medium18, F::Regular16, Color::CYAN };
        if (p.weeks < p.planWeeks) {
            snprintf(mText[5], sizeof(mText[5]), "%u of its %u weeks", static_cast<unsigned>(p.weeks),
                     static_cast<unsigned>(p.planWeeks));
        } else {
            snprintf(mText[5], sizeof(mText[5]), "%u weeks", static_cast<unsigned>(p.weeks));
        }
        lines[n++] = Line{ mText[5], F::Italic18, F::Italic18, Color::TEAL };
    } else if (p.problem[0] != '\0') {
        snprintf(mText[4], sizeof(mText[4]), "%s", p.problem);
        lines[n++] = Line{ mText[4], F::Medium18, F::Regular16, Color::YELLOW };
    }
    mStack.set(lines, n);
    mButtons->setR1(p.ok ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE);
}

void PlanConfirmScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::R1:
            if (mModel.getPlanPreview().ok) {
                mModel.newPlan(Act::Apply, mModel.draft());
                mModel.setDraftReady(false);
                ScreenManager::instance().goTo(ScreenId::PlanToday);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::NewPlan);
            break;
        default: break;
    }
}

// -- The phone's new plan ----------------------------------------------------------------

PlanAskScreen::PlanAskScreen(Model& model) : Screen(model) {}
PlanAskScreen::~PlanAskScreen() = default;

void PlanAskScreen::build()
{
    mItems[ID_START] = { Style::Tip, "Start it" };
    mItems[ID_KEEP]  = { Style::Tip, "Keep mine" };
    for (uint16_t i = 0; i < ID_COUNT; ++i) {
        mItems[i].tip     = mTip[i];
        mItems[i].itemTip = "";
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, ID_COUNT);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "NEW PLAN?");
}

void PlanAskScreen::onShow()
{
    mModel.resetIdleTimer();
    onPlanSetup();
    mModel.requestPlan(CustomMessage::PlanRequest::Setup);
}

void PlanAskScreen::onPlanSetup()
{
    const Info& phone = mModel.getPhoneSetup();
    const Info& mine  = mModel.getCurrentSetup();
    // "10K, level 2"; the plan's name alone when that is too long for the hint.
    const Info* const setups[ID_COUNT] = { &phone, &mine };
    for (uint16_t i = 0; i < ID_COUNT; ++i) {
        snprintf(mTip[i], sizeof(mTip[0]), "%s, level %u", NewPlanText::planName(setups[i]->plan),
                 static_cast<unsigned>(setups[i]->level));
        if (WorkoutUi::textWidth(mTip[i], Theme::font(F::Italic18)) > 180) {
            snprintf(mTip[i], sizeof(mTip[0]), "%s", NewPlanText::planName(setups[i]->plan));
        }
        WorkoutUi::fitWidth(mTip[i], sizeof(mTip[i]), Theme::font(F::Italic18), 180);
    }
    mMenu->refresh();
}

void PlanAskScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            if (!mModel.phoneAsks()) {
                break;   // the set-up has not arrived yet
            }
            mModel.newPlan(mMenu->selected() == ID_START ? Act::Apply : Act::KeepCurrent, mModel.getPhoneSetup());
            ScreenManager::instance().goTo(ScreenId::PlanToday);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::PlanToday);
            break;
        default: break;
    }
}
