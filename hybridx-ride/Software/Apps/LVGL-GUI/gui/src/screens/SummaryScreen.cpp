/**
 ******************************************************************************
 * @file    SummaryScreen.cpp
 * @brief   What the ride came to (see SummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/SummaryScreen.hpp"

#include <cstdio>

#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RideText.hpp"

using namespace SDK::GUI;

static_assert(Ride::kLapsPerPage == 3, "the lap rows are laid out for three to a screen");

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
    return static_cast<uint8_t>(fixedPages() + (laps + kLapsPerScreen - 1) / kLapsPerScreen);
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

    // Outdoors the Ride page sits second; the rest keep their order.
    const bool outdoor = mModel.saved().outdoor != 0;
    const uint8_t fixed = fixedPages();
    char title[24];
    if (page == 0) {
        std::snprintf(title, sizeof(title), "%s", Strings::kSummary);
        buildOverview();
    } else if (outdoor && page == 1) {
        std::snprintf(title, sizeof(title), "%s", Strings::kRideSummary);
        buildRide();
    } else if (page == fixed - 2) {
        std::snprintf(title, sizeof(title), "%s", Strings::kZones);
        buildZones();
    } else if (page == fixed - 1) {
        std::snprintf(title, sizeof(title), "%s", Strings::kHeartRate);
        buildHeart();
    } else {
        std::snprintf(title, sizeof(title), "Laps %u/%u", static_cast<unsigned>(page - fixed + 1),
                      static_cast<unsigned>(pageCount() - fixed));
        buildLaps();
        // Empty until the service answers.
        mWantedFirst = static_cast<uint8_t>((page - fixed) * kLapsPerScreen);
        mModel.requestLaps(mWantedFirst);
    }
    mTitle->setText(title);
}

void SummaryScreen::buildOverview()
{
    using F = Theme::Font;
    const Ride::SavedSession& saved = mModel.saved();
    char buf[40];
    char kcal[16];

    Theme::label(mPageRoot, F::Regular14, "Total time", 0, 48, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Ride::formatClock(buf, sizeof(buf), saved.activeMs);
    Theme::label(mPageRoot, F::SemiBold40, buf, 0, 64, 240);

    Ride::formatKcal(kcal, sizeof(kcal), saved.summary.kcalX10);
    if (saved.outdoor) {
        // Outdoors: the distance, then the laps and calories together.
        char dist[16];
        Ride::formatDistance(dist, sizeof(dist), saved.summary.distCm, mModel.imperial());
        std::snprintf(buf, sizeof(buf), "%s %s", dist, mModel.imperial() ? Strings::kMiles : Strings::kKm);
        Theme::label(mPageRoot, F::Medium18, buf, 0, 130, 240);
        std::snprintf(buf, sizeof(buf), "%u %s  %s kcal", static_cast<unsigned>(saved.summary.laps),
                      saved.summary.laps == 1 ? "lap" : "laps", kcal);
        Theme::label(mPageRoot, F::Regular14, buf, 0, 162, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        return;
    }

    std::snprintf(buf, sizeof(buf), "%u %s", static_cast<unsigned>(saved.summary.laps),
                  saved.summary.laps == 1 ? "lap" : "laps");
    Theme::label(mPageRoot, F::Medium18, buf, 0, 130, 240);

    std::snprintf(buf, sizeof(buf), "%s kcal", kcal);
    Theme::label(mPageRoot, F::Regular14, buf, 0, 162, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
}

void SummaryScreen::buildRide()
{
    using F = Theme::Font;
    const Ride::SessionSummary& s = mModel.saved().summary;
    const bool imperial = mModel.imperial();
    char captions[3][24];
    std::snprintf(captions[0], sizeof(captions[0]), "Average (%s)", imperial ? Strings::kMph : Strings::kKmh);
    std::snprintf(captions[1], sizeof(captions[1]), "Top speed (%s)", imperial ? Strings::kMph : Strings::kKmh);
    std::snprintf(captions[2], sizeof(captions[2]), "Climb (%s)", imperial ? Strings::kFeet : Strings::kMetres);

    char value[3][16];
    Ride::formatSpeed(value[0], sizeof(value[0]), s.avgSpeedMmS, imperial);
    Ride::formatSpeed(value[1], sizeof(value[1]), s.maxSpeedMmS, imperial);
    Ride::formatClimb(value[2], sizeof(value[2]), s.ascentCm, imperial);

    for (uint8_t i = 0; i < 3; ++i) {
        const int32_t y = 46 + static_cast<int32_t>(i) * 52;
        Theme::label(mPageRoot, F::Regular14, captions[i], 0, y, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        Theme::label(mPageRoot, F::Medium25, value[i], 0, y + 17, 240);
    }
}

void SummaryScreen::buildZones()
{
    using F = Theme::Font;
    const Ride::SessionSummary& s = mModel.saved().summary;

    // Zone 5 at the top, as the live face; a zone with no time is grey.
    for (uint8_t i = 0; i < 5; ++i) {
        const uint8_t zone = static_cast<uint8_t>(5u - i);
        char name[8];
        char time[16];
        Ride::formatZone(name, sizeof(name), zone);
        Ride::formatClock(time, sizeof(time), s.zoneMs[zone]);
        const uint32_t color = s.zoneMs[zone] > 0 ? Theme::zoneColor(zone) : Color::GRAY;
        const int32_t  y     = 48 + static_cast<int32_t>(i) * 28;
        Theme::label(mPageRoot, F::Medium18, name, 50, y, 50, LV_TEXT_ALIGN_LEFT, color);
        Theme::label(mPageRoot, F::Medium18, time, 100, y, 90, LV_TEXT_ALIGN_RIGHT, color);
    }
}

void SummaryScreen::buildHeart()
{
    using F = Theme::Font;
    const Ride::SessionSummary& s = mModel.saved().summary;
    static const char* const captions[3] = { "Average (bpm)", "Highest (bpm)", "Calories (kcal)" };

    char value[3][16];
    Ride::formatBpm(value[0], sizeof(value[0]), s.hrAvg);
    Ride::formatBpm(value[1], sizeof(value[1]), s.hrMax);
    Ride::formatKcal(value[2], sizeof(value[2]), s.kcalX10);

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

void SummaryScreen::onLapsPage(const Ride::LapsPageData& page)
{
    if (mPage >= fixedPages() && page.first == mWantedFirst) {
        fillLaps(page);
    }
}

void SummaryScreen::fillLaps(const Ride::LapsPageData& page)
{
    for (uint8_t i = 0; i < kLapsPerScreen; ++i) {
        if (i >= page.count) {
            Theme::assign(mLapName[i], "");
            Theme::assign(mLapTime[i], "");
            Theme::assign(mLapAvg[i], "");
            Theme::assign(mLapRec[i], "");
            continue;
        }
        const Ride::LapRow& row = page.rows[i];
        char buf[32];
        char part[16];

        // "Lap 3", or "Auto 3" for one the auto lap ended ("Lap 3 auto" does not fit).
        std::snprintf(buf, sizeof(buf), "%s %u", row.autoEnded ? "Auto" : "Lap",
                      static_cast<unsigned>(page.first + i + 1u));
        Theme::assign(mLapName[i], buf);

        Ride::formatClock(buf, sizeof(buf), row.durationMs);
        Theme::assign(mLapTime[i], buf);

        Ride::formatBpm(part, sizeof(part), row.hrAvg);
        std::snprintf(buf, sizeof(buf), "avg %s", part);
        Theme::assign(mLapAvg[i], buf);

        if (mModel.saved().outdoor) {
            // Outdoors the lap's distance in place of its calories.
            Ride::formatDistance(part, sizeof(part), row.distCm, mModel.imperial());
            std::snprintf(buf, sizeof(buf), "%s %s", part, mModel.imperial() ? Strings::kMiles : Strings::kKm);
        } else {
            std::snprintf(buf, sizeof(buf), "%u kcal", static_cast<unsigned>(row.kcal));
        }
        Theme::assign(mLapRec[i], buf);
    }
}
