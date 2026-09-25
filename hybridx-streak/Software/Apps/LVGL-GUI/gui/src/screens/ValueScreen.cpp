/**
 ******************************************************************************
 * @file    ValueScreen.cpp
 * @brief   One setting's choices (see the header).
 *
 * Model::editField picks the setting, in SettingsScreen's order: target,
 * week start, what counts, shortest session.
 ******************************************************************************
 */

#include "gui/screens/ValueScreen.hpp"

#include <cstdio>

#include "gui/copy/Coach.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = Widgets::Wheel::Item::Style;

namespace
{
enum : uint8_t { kTarget, kWeekStart, kCounts, kMinimum };
constexpr uint8_t kMinimums[] = {0, 5, 10, 15, 20, 30, 45, 60};

Streak::Goal effective(const Model& m)
{
    const CustomMessage::GoalData& g = m.goal();
    return g.hasPending ? g.pending : g.goal;
}

uint8_t current(const Streak::Goal& g, uint8_t field)
{
    switch (field) {
        case kTarget:    return g.target;
        case kWeekStart: return g.weekStart;
        case kCounts:    return g.scope;
        default:         return g.minMinutes;
    }
}
} // namespace

void ValueScreen::build()
{
    const uint8_t field = mModel.editField;
    const char*   title = "Weekly target";
    mCount              = 0;
    switch (field) {
        case kTarget:
            for (uint8_t t = 1; t <= 7; ++t) {
                snprintf(mText[mCount], sizeof(mText[0]), "%u a week", static_cast<unsigned>(t));
                mValues[mCount++] = t;
            }
            break;
        case kWeekStart:
            title = "Week starts";
            for (uint8_t d = 1; d <= 7; ++d) {   // Monday first
                snprintf(mText[mCount], sizeof(mText[0]), "%s", Coach::dayLong(static_cast<uint8_t>(d % 7)));
                mValues[mCount++] = static_cast<uint8_t>(d % 7);
            }
            break;
        case kCounts:
            title = "What counts";
            snprintf(mText[mCount], sizeof(mText[0]), "Everything");
            mValues[mCount++] = Streak::kScopeAny;
            for (uint8_t k = 0; k < Streak::kKindCount && mCount < kMaxChoices; ++k) {
                snprintf(mText[mCount], sizeof(mText[0]), "%s", Coach::scopeName(k));
                mValues[mCount++] = k;
            }
            break;
        default:
            title = "Shortest";
            for (uint8_t m : kMinimums) {
                if (m == 0) {
                    snprintf(mText[mCount], sizeof(mText[0]), "Any length");
                } else {
                    snprintf(mText[mCount], sizeof(mText[0]), "%u min", static_cast<unsigned>(m));
                }
                mValues[mCount++] = m;
            }
            break;
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        mItems[i] = { Style::Simple, mText[i] };
    }
    mTitle   = std::make_unique<Widgets::Title>(mRoot, title);
    mMenu    = std::make_unique<Widgets::Wheel>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::GREEN, Widgets::Buttons::WHITE);
}

void ValueScreen::onShow()
{
    const uint8_t now = current(effective(mModel), mModel.editField);
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mValues[i] == now) {
            mMenu->select(i);
        }
    }
    mModel.resetIdleTimer();
}

void ValueScreen::onHide() {}

void ValueScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            Streak::Goal  g = effective(mModel);
            const uint8_t v = mValues[mMenu->selected()];
            switch (mModel.editField) {
                case kTarget:    g.target = v; break;
                case kWeekStart: g.weekStart = v; break;
                case kCounts:    g.scope = v; break;
                default:         g.minMinutes = v; break;
            }
            mModel.setGoal(g);
            ScreenManager::instance().goTo(ScreenId::Settings);
            break;
        }
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Settings);
            break;
        default:
            break;
    }
}
