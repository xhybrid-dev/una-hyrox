/**
 ******************************************************************************
 * @file    SummaryScreen.cpp
 * @brief   What the workout came to (see SummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/SummaryScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "WorkoutText.hpp"

using namespace SDK::GUI;

static_assert(Workout::kLapsPerPage == 3, "the lap rows are laid out for three to a screen");

namespace
{
constexpr int32_t kColX = 40;    // the text column: clear of the arcs at the top
constexpr int32_t kColW = 160;
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
            std::snprintf(title, sizeof(title), "%s", Strings::kZones);
            buildZones();
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
    const Workout::SavedSession& saved = mModel.saved();
    char buf[40];
    char kcal[16];

    Theme::label(mPageRoot, F::Regular14, "Total time", 0, 48, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Workout::formatClock(buf, sizeof(buf), saved.activeMs);
    Theme::label(mPageRoot, F::SemiBold40, buf, 0, 64, 240);

    std::snprintf(buf, sizeof(buf), "%u %s", static_cast<unsigned>(saved.summary.laps),
                  saved.summary.laps == 1 ? "lap" : "laps");
    Theme::label(mPageRoot, F::Medium18, buf, 0, 130, 240);

    Workout::formatKcal(kcal, sizeof(kcal), saved.summary.kcalX10);
    std::snprintf(buf, sizeof(buf), "%s kcal", kcal);
    Theme::label(mPageRoot, F::Regular14, buf, 0, 162, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void SummaryScreen::buildZones()
{
    using F = Theme::Font;
    const Workout::SessionSummary& s = mModel.saved().summary;

    // Zone 5 at the top, as the live face; a zone with no time is grey.
    for (uint8_t i = 0; i < 5; ++i) {
        const uint8_t zone = static_cast<uint8_t>(5u - i);
        char name[8];
        char time[16];
        Workout::formatZone(name, sizeof(name), zone);
        Workout::formatClock(time, sizeof(time), s.zoneMs[zone]);
        const uint32_t color = s.zoneMs[zone] > 0 ? Theme::zoneColor(zone) : Color::GRAY;
        const int32_t  y     = 48 + static_cast<int32_t>(i) * 28;
        Theme::label(mPageRoot, F::Medium18, name, 50, y, 50, LV_TEXT_ALIGN_LEFT, color);
        Theme::label(mPageRoot, F::Medium18, time, 100, y, 90, LV_TEXT_ALIGN_RIGHT, color);
    }
}

void SummaryScreen::buildHeart()
{
    using F = Theme::Font;
    const Workout::SessionSummary& s = mModel.saved().summary;
    static const char* const captions[3] = { "Average (bpm)", "Highest (bpm)", "Calories (kcal)" };

    char value[3][16];
    Workout::formatBpm(value[0], sizeof(value[0]), s.hrAvg);
    Workout::formatBpm(value[1], sizeof(value[1]), s.hrMax);
    Workout::formatKcal(value[2], sizeof(value[2]), s.kcalX10);

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
        // "Auto 64" | "99:59:59" over "avg 255" | "65535 kcal".
        mLapName[i] = Theme::label(mPageRoot, F::Medium18, "", kColX, y, 76, LV_TEXT_ALIGN_LEFT);
        mLapTime[i] = Theme::label(mPageRoot, F::Medium18, "", kColX + 76, y, kColW - 76, LV_TEXT_ALIGN_RIGHT);
        mLapAvg[i]  = Theme::label(mPageRoot, F::Regular14, "", kColX, y + 25, 80, LV_TEXT_ALIGN_LEFT, Color::GRAY);
        mLapRec[i]  = Theme::label(mPageRoot, F::Regular14, "", kColX + 80, y + 25, kColW - 80,
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

void SummaryScreen::onLapsPage(const Workout::LapsPageData& page)
{
    if (mPage >= kFixedPages && page.first == mWantedFirst) {
        fillLaps(page);
    }
}

void SummaryScreen::fillLaps(const Workout::LapsPageData& page)
{
    for (uint8_t i = 0; i < kLapsPerScreen; ++i) {
        if (i >= page.count) {
            Theme::assign(mLapName[i], "");
            Theme::assign(mLapTime[i], "");
            Theme::assign(mLapAvg[i], "");
            Theme::assign(mLapRec[i], "");
            continue;
        }
        const Workout::LapRow& row = page.rows[i];
        char buf[32];
        char part[16];

        // "Lap 3", or "Auto 3" for one the auto lap ended ("Lap 3 auto" does not fit).
        std::snprintf(buf, sizeof(buf), "%s %u", row.autoEnded ? "Auto" : "Lap",
                      static_cast<unsigned>(page.first + i + 1u));
        Theme::assign(mLapName[i], buf);

        Workout::formatClock(buf, sizeof(buf), row.durationMs);
        Theme::assign(mLapTime[i], buf);

        Workout::formatBpm(part, sizeof(part), row.hrAvg);
        std::snprintf(buf, sizeof(buf), "avg %s", part);
        Theme::assign(mLapAvg[i], buf);

        std::snprintf(buf, sizeof(buf), "%u kcal", static_cast<unsigned>(row.kcal));
        Theme::assign(mLapRec[i], buf);
    }
}
