/**
 ******************************************************************************
 * @file    PacesScreen.cpp
 * @brief   Paces (see the header).
 ******************************************************************************
 */

#include "gui/screens/PacesScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include "Commands.hpp"
#include "TimeText.hpp"

#include <cstdio>
#include <cstring>

namespace Color = SDK::GUI::Color;
using F = Theme::Font;
using Line = TextStack::Line;

namespace
{
/// "4:40" for a pace in s/km, per mile when @p imperial.
void pace(char* buf, size_t cap, uint32_t sPerKm, bool imperial)
{
    const uint32_t s = imperial ? static_cast<uint32_t>((static_cast<uint64_t>(sPerKm) * 1609344u + 500000u) / 1000000u)
                                : sPerKm;
    snprintf(buf, cap, "%u:%02u", static_cast<unsigned>(s / 60), static_cast<unsigned>(s % 60));
}

/// Word-wrap @p text into lines of at most @p width pixels in @p font; returns the count.
uint8_t wrap(const char* text, char (*lines)[48], uint8_t maxLines, const lv_font_t* font, int32_t width)
{
    uint8_t n = 0;
    while (*text != '\0' && n < maxLines) {
        while (*text == ' ') {
            ++text;
        }
        char* line = lines[n];
        size_t len = 0;
        size_t fit = 0;
        const char* p = text;
        // The longest run of whole words that fits; a word too long alone is cut by the stack.
        while (true) {
            const char* end = p;
            while (*end != '\0' && *end != ' ') {
                ++end;
            }
            const size_t add = static_cast<size_t>(end - text);
            if (add >= 47) {
                break;
            }
            std::memcpy(line, text, add);
            line[add] = '\0';
            if (fit > 0 && WorkoutUi::textWidth(line, font) > width) {
                break;
            }
            fit = add;
            len = add;
            if (*end == '\0') {
                break;
            }
            p = end + 1;
        }
        if (fit == 0) {
            break;
        }
        line[len] = '\0';
        text += len;
        ++n;
    }
    return n;
}
} // namespace

PacesScreen::PacesScreen(Model& model)
    : Screen(model)
{
}

PacesScreen::~PacesScreen() = default;

void PacesScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mTitleText);
    mStack.init(mRoot);
}

void PacesScreen::onShow()
{
    mModel.resetIdleTimer();
    show(0);
    mModel.requestPlan(CustomMessage::PlanRequest::Paces);
}

void PacesScreen::onPlanPaces()
{
    show(mPage < pages() ? mPage : 0);
}

uint8_t PacesScreen::pages() const
{
    if (!mModel.getPlanPaces().ok) {
        return 1;
    }
    char buf[200];
    note(buf, sizeof(buf));
    return buf[0] != '\0' ? 4 : 3;
}

void PacesScreen::note(char* buf, size_t cap) const
{
    const CustomMessage::PlanPacesInfo& p = mModel.getPlanPaces();
    buf[0] = '\0';
    size_t len = 0;
    auto add = [&](const char* text) {
        if (len < cap) {
            const int n = snprintf(buf + len, cap - len, "%s%s", len > 0 ? " " : "", text);
            len += n > 0 ? static_cast<size_t>(n) : 0;
        }
    };
    if (!p.ok) {
        return;
    }
    const uint16_t gap = p.tenK > p.threshold ? static_cast<uint16_t>(p.tenK - p.threshold)
                                              : static_cast<uint16_t>(p.threshold - p.tenK);
    if (gap <= kNearSPerKm) {
        add("Threshold is about your 10K pace.");   // PLANS.md 2.3
    }
    if (p.gap && p.halfAs5kSec != 0) {
        char t[16];
        XTraining::TimeText::duration(p.halfAs5kSec, t, sizeof(t));
        char s[120];
        snprintf(s, sizeof(s), "Your half matches a %s 5K: long runs matter most for you.", t);   // PLANS.md 2.4
        add(s);
    }
    if (p.halfSec != 0 && !p.halfUsed) {
        add("Your half is over six months old: the paces use your 5K.");
    }
}

void PacesScreen::show(uint8_t page)
{
    const CustomMessage::PlanPacesInfo& p = mModel.getPlanPaces();
    const bool imperial = mModel.isUnitsImperial();
    for (auto& t : mText) {
        t[0] = '\0';
    }
    Line lines[TextStack::kMaxLines];
    uint8_t n = 0;
    mPage = page;
    if (!p.ok) {
        snprintf(mTitleText, sizeof(mTitleText), "PACES");
        lines[n++] = Line{ "No paces yet", F::SemiBold25, F::SemiBold20, Color::WHITE };
        lines[n++] = Line{ "Set your 5K in", F::Medium18, F::Regular16, Color::GRAY };
        lines[n++] = Line{ "the UNA app", F::Medium18, F::Regular16, Color::GRAY };
    } else if (page == 0) {
        snprintf(mTitleText, sizeof(mTitleText), "PACES %s", imperial ? "/MI" : "/KM");
        char a[12];
        char b[12];
        pace(a, sizeof(a), p.easyFast, imperial);
        pace(b, sizeof(b), p.easySlow, imperial);
        snprintf(mText[0], sizeof(mText[0]), "Easy %s-%s", a, b);
        const struct { const char* name; uint16_t v; } rows[] = {
            { "Half", p.half }, { "Threshold", p.threshold }, { "10K", p.tenK },
            { "Interval", p.interval }, { "Repetition", p.repetition },
        };
        lines[n++] = Line{ mText[0], F::Medium18, F::Regular16, Color::WHITE };
        for (uint8_t i = 0; i < 5; ++i) {
            pace(a, sizeof(a), rows[i].v, imperial);
            snprintf(mText[i + 1], sizeof(mText[i + 1]), "%s %s", rows[i].name, a);
            lines[n++] = Line{ mText[i + 1], F::Medium18, F::Regular16, Color::WHITE };
        }
    } else if (page == 1) {
        // The times the paces come from, each with the day it was first seen.
        snprintf(mTitleText, sizeof(mTitleText), "YOUR TIMES");
        char t[16];
        char d[24];
        XTraining::TimeText::duration(p.fiveKSec, t, sizeof(t));
        XTraining::TimeText::dayDate(p.fiveKDate, d, sizeof(d));
        snprintf(mText[0], sizeof(mText[0]), "5K %s", t);
        snprintf(mText[1], sizeof(mText[1]), "%s", d);
        lines[n++] = Line{ mText[0], F::SemiBold25, F::SemiBold20, Color::WHITE };
        lines[n++] = Line{ mText[1], F::Italic18, F::Italic18, Color::GRAY };
        if (p.halfUsed) {
            XTraining::TimeText::duration(p.halfSec, t, sizeof(t));
            XTraining::TimeText::dayDate(p.halfDate, d, sizeof(d));
            snprintf(mText[2], sizeof(mText[2]), "Half %s", t);
            snprintf(mText[3], sizeof(mText[3]), "%s", d);
            lines[n++] = Line{ mText[2], F::SemiBold25, F::SemiBold20, Color::WHITE };
            lines[n++] = Line{ mText[3], F::Italic18, F::Italic18, Color::GRAY };
        }
    } else if (page == 2) {
        snprintf(mTitleText, sizeof(mTitleText), "PREDICTED");
        char t[16];
        XTraining::TimeText::duration(p.predicted10kSec, t, sizeof(t));
        snprintf(mText[3], sizeof(mText[3]), "10K %s", t);
        XTraining::TimeText::duration(p.predictedHalfSec, t, sizeof(t));
        snprintf(mText[4], sizeof(mText[4]), "Half %s", t);
        XTraining::TimeText::duration(p.predictedMarathonSec, t, sizeof(t));
        snprintf(mText[5], sizeof(mText[5]), "Marathon %s", t);
        lines[n++] = Line{ mText[3], F::Medium25, F::Medium18, Color::WHITE };
        lines[n++] = Line{ mText[4], F::Medium25, F::Medium18, Color::WHITE };
        lines[n++] = Line{ mText[5], F::Medium25, F::Medium18, Color::WHITE };
    } else {
        snprintf(mTitleText, sizeof(mTitleText), "A NOTE");
        char buf[200];
        note(buf, sizeof(buf));
        const uint8_t k = wrap(buf, mText, 6, Theme::font(F::Regular16), 170);
        for (uint8_t i = 0; i < k; ++i) {
            lines[n++] = Line{ mText[i], F::Regular16, F::Regular16, Color::WHITE };
        }
    }
    mTitle->setText(mTitleText);
    mStack.set(lines, n);
    const uint8_t last = static_cast<uint8_t>(pages() - 1);
    mButtons->setL1(mPage > 0 ? Widgets::Buttons::WHITE : Widgets::Buttons::NONE);
    mButtons->setL2(mPage < last ? Widgets::Buttons::WHITE : Widgets::Buttons::NONE);
}

void PacesScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            if (mPage > 0) {
                show(static_cast<uint8_t>(mPage - 1));
            }
            break;
        case Btn::L2:
            if (mPage + 1 < pages()) {
                show(static_cast<uint8_t>(mPage + 1));
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
}
