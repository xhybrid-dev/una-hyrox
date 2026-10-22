/**
 ******************************************************************************
 * @file    SummaryScreen.cpp
 * @brief   What the session came to (see SummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/SummaryScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "SessionText.hpp"

using namespace SDK::GUI;

static_assert(Sauna::kLapsPerPage == 3, "the lap rows are laid out for three to a screen");

namespace
{
constexpr int32_t kColX = 40;    // the text column: clear of the arcs at the top
constexpr int32_t kColW = 160;

/// A drop in bpm as the screens write it: "-23", "+4", "0".
void dropText(char* out, size_t cap, int16_t drop)
{
    Sauna::Recovery r;
    r.state = Sauna::Recovery::State::Done;
    r.drop  = drop;
    Sauna::formatRecovery(out, cap, r);
}
} // namespace

SummaryScreen::SummaryScreen(Model& model)
    : Screen(model)
{
}

uint8_t SummaryScreen::pageCount() const
{
    const uint8_t laps = mModel.saved().summary.laps;
    return static_cast<uint8_t>(kFixedPages + (laps + kLapsPerScreen - 1) / kLapsPerScreen);
}

void SummaryScreen::build()
{
    mTitle   = std::make_unique<Widgets::Title>(mRoot, Strings::kSummary);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE, Widgets::Buttons::NONE,
                  Widgets::Buttons::WHITE);
}

void SummaryScreen::onShow()
{
    mModel.setIdleTimeout();
    showPage(0);
}

void SummaryScreen::showPage(uint8_t page)
{
    mPage = page;

    if (mPageRoot) {
        lv_obj_delete(mPageRoot);
        mPageRoot = nullptr;
    }
    for (uint8_t i = 0; i < kLapsPerScreen; ++i) {
        mLapName[i] = mLapTime[i] = mLapAvg[i] = mLapRec[i] = nullptr;
    }
    mPageRoot = Theme::container(mRoot, 0, 0, 240, 240);
    // Under the title and the bezel hints, which were made first.
    lv_obj_move_to_index(mPageRoot, 0);

    char title[24];
    switch (page) {
        case 0:
            std::snprintf(title, sizeof(title), "%s", Strings::kSummary);
            buildOverview();
            break;
        case 1:
            std::snprintf(title, sizeof(title), "%s", "Time");
            buildPhases();
            break;
        case 2:
            std::snprintf(title, sizeof(title), "%s", Strings::kHeartRate);
            buildHeart();
            break;
        default:
            std::snprintf(title, sizeof(title), "Laps %u/%u", static_cast<unsigned>(page - kFixedPages + 1),
                          static_cast<unsigned>(pageCount() - kFixedPages));
            buildLaps();
            // Empty until the service answers.
            mWantedFirst = static_cast<uint8_t>((page - kFixedPages) * kLapsPerScreen);
            mModel.requestLaps(mWantedFirst);
            break;
    }
    mTitle->setText(title);
}

void SummaryScreen::buildOverview()
{
    using F = Theme::Font;
    const Sauna::SavedSession& saved = mModel.saved();
    char buf[40];

    Theme::label(mPageRoot, F::Regular14, "Total time", 0, 48, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Sauna::formatClock(buf, sizeof(buf), saved.activeMs);
    Theme::label(mPageRoot, F::SemiBold40, buf, 0, 64, 240);

    std::snprintf(buf, sizeof(buf), "%u %s, %u %s", static_cast<unsigned>(saved.summary.rounds),
                  saved.summary.rounds == 1 ? "round" : "rounds", static_cast<unsigned>(saved.summary.laps),
                  saved.summary.laps == 1 ? "lap" : "laps");
    Theme::label(mPageRoot, F::Medium18, buf, 0, 130, 240);

    if (saved.summary.hasRecovery) {
        char drop[16];
        dropText(drop, sizeof(drop), saved.summary.bestDrop);
        std::snprintf(buf, sizeof(buf), "best recovery %s", drop);
        Theme::label(mPageRoot, F::Regular14, buf, 0, 162, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    }
}

void SummaryScreen::buildPhases()
{
    using F = Theme::Font;
    const Sauna::SessionSummary& s = mModel.saved().summary;

    // Each phase the session used: Sauna always, the others if they took time.
    struct Row { uint8_t phase; uint32_t ms; };
    Row rows[kPhaseRows];
    uint8_t n = 0;
    rows[n++] = { 0, s.saunaMs };
    if (s.plungeMs > 0) {
        rows[n++] = { 1, s.plungeMs };
    }
    if (s.restMs > 0) {
        rows[n++] = { 2, s.restMs };
    }

    const int32_t y0 = 70 + static_cast<int32_t>(kPhaseRows - n) * 17;
    for (uint8_t i = 0; i < n; ++i) {
        char buf[16];
        Sauna::formatClock(buf, sizeof(buf), rows[i].ms);
        const uint32_t color = Theme::phaseColor(static_cast<Sauna::Phase>(rows[i].phase));
        const int32_t  y     = y0 + static_cast<int32_t>(i) * 34;
        Theme::label(mPageRoot, F::Medium18, Strings::phase(rows[i].phase), kColX, y, 80, LV_TEXT_ALIGN_LEFT, color);
        Theme::label(mPageRoot, F::Medium18, buf, kColX + 80, y, kColW - 80, LV_TEXT_ALIGN_RIGHT, color);
    }
}

void SummaryScreen::buildHeart()
{
    using F = Theme::Font;
    const Sauna::SessionSummary& s = mModel.saved().summary;
    static const char* const captions[3] = { "Average (bpm)", "Highest (bpm)", "Best recovery (bpm)" };

    char value[3][16];
    Sauna::formatBpm(value[0], sizeof(value[0]), s.hrAvg);
    Sauna::formatBpm(value[1], sizeof(value[1]), s.hrMax);
    if (s.hasRecovery) {
        dropText(value[2], sizeof(value[2]), s.bestDrop);
    } else {
        std::snprintf(value[2], sizeof(value[2]), "%s", Strings::kNoValue);
    }

    for (uint8_t i = 0; i < 3; ++i) {
        const int32_t y = 46 + static_cast<int32_t>(i) * 52;
        Theme::label(mPageRoot, F::Regular14, captions[i], 0, y, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        Theme::label(mPageRoot, F::Medium25, value[i], 0, y + 17, 240);
    }
}

void SummaryScreen::buildLaps()
{
    using F = Theme::Font;
    for (uint8_t i = 0; i < kLapsPerScreen; ++i) {
        const int32_t y = 46 + static_cast<int32_t>(i) * 50;
        mLapName[i] = Theme::label(mPageRoot, F::Medium18, "", kColX, y, 90, LV_TEXT_ALIGN_LEFT);
        mLapTime[i] = Theme::label(mPageRoot, F::Medium18, "", kColX + 90, y, kColW - 90, LV_TEXT_ALIGN_RIGHT);
        mLapAvg[i]  = Theme::label(mPageRoot, F::Regular14, "", kColX, y + 25, 70, LV_TEXT_ALIGN_LEFT, Color::GRAY);
        mLapRec[i]  = Theme::label(mPageRoot, F::Regular14, "", kColX + 70, y + 25, kColW - 70,
                                   LV_TEXT_ALIGN_RIGHT, Color::GRAY);
    }
}

void SummaryScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const uint8_t n = pageCount();
    switch (code) {
        case Btn::L1: showPage(static_cast<uint8_t>((mPage + n - 1) % n)); break;
        case Btn::L2: showPage(static_cast<uint8_t>((mPage + 1) % n)); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}

void SummaryScreen::onIdleTimeout()
{
    mModel.exitApp();
}

void SummaryScreen::onLapsPage(const Sauna::LapsPageData& page)
{
    if (mPage >= kFixedPages && page.first == mWantedFirst) {
        fillLaps(page);
    }
}

void SummaryScreen::fillLaps(const Sauna::LapsPageData& page)
{
    for (uint8_t i = 0; i < kLapsPerScreen; ++i) {
        if (i >= page.count) {
            Theme::assign(mLapName[i], "");
            Theme::assign(mLapTime[i], "");
            Theme::assign(mLapAvg[i], "");
            Theme::assign(mLapRec[i], "");
            continue;
        }
        const Sauna::LapRow& row = page.rows[i];
        char buf[32];
        char part[16];

        std::snprintf(buf, sizeof(buf), "%s %u", Strings::phase(row.phase), static_cast<unsigned>(row.round));
        Theme::setText(mLapName[i], buf, Theme::phaseColor(static_cast<Sauna::Phase>(row.phase)));

        Sauna::formatClock(buf, sizeof(buf), row.durationMs);
        Theme::assign(mLapTime[i], buf);

        Sauna::formatBpm(part, sizeof(part), row.hrAvg);
        std::snprintf(buf, sizeof(buf), "avg %s", part);
        Theme::assign(mLapAvg[i], buf);

        if (row.recoveryState == static_cast<uint8_t>(Sauna::Recovery::State::Done)) {
            dropText(part, sizeof(part), row.recoveryDrop);
        } else {
            std::snprintf(part, sizeof(part), "%s", Strings::kNoValue);
        }
        std::snprintf(buf, sizeof(buf), "recovery %s", part);
        Theme::assign(mLapRec[i], buf);
    }
}
