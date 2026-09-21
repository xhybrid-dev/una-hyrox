/**
 ******************************************************************************
 * @file    TrackSummaryScreen.cpp
 * @brief   Race summary (see TrackSummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackSummaryScreen.hpp"

#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include "RaceModel.hpp"

using namespace SDK::GUI;

TrackSummaryScreen::TrackSummaryScreen(Model& model)
    : Screen(model)
{
}

void TrackSummaryScreen::build()
{
    using F = Theme::Font;

    // The rows are a two-column table between x = 34 and x = 206. That is the
    // width the display still has at the bottom row: a round 240 px screen is
    // only about 175 px across at y = 200, and the first version of this screen
    // ran "HR 146 avg 156 max" off both edges.
    mHeading = Theme::label(mRoot, F::Italic18, "", 0, 42, 240);
    for (uint8_t i = 0; i < kRowCount; ++i) {
        const int32_t y = 64 + i * 24;
        mRowName[i]  = Theme::label(mRoot, F::Regular16, "", 34, y, 110,
                                    LV_TEXT_ALIGN_LEFT, Color::GRAY);
        mRowValue[i] = Theme::label(mRoot, F::Regular16, "", 96, y, 110,
                                    LV_TEXT_ALIGN_RIGHT, Color::WHITE);
    }

    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Summary");
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::NONE, Widgets::Buttons::AMBER);
}

void TrackSummaryScreen::setRow(uint8_t i, const char* name, const char* value)
{
    if (i >= kRowCount) {
        return;  // no MMU: never index past the arrays
    }
    lv_label_set_text(mRowName[i], (name != nullptr) ? name : "");
    lv_label_set_text(mRowValue[i], (name != nullptr && value != nullptr) ? value : "");
}

void TrackSummaryScreen::onShow()
{
    mModel.resetIdleTimer();
    mPage = 0;
    // The summary may not have arrived yet if the GUI restarted; asking is
    // cheap and idempotent.
    mModel.requestSummary();
    redraw();
}

uint8_t TrackSummaryScreen::pageCount() const
{
    const uint8_t segments = mModel.getSummary().count;
    const uint8_t splitPages =
        static_cast<uint8_t>((segments + kRowsPerPage - 1) / kRowsPerPage);
    return static_cast<uint8_t>(1u + splitPages);
}

void TrackSummaryScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const uint8_t pages = pageCount();

    switch (code) {
    case Btn::L1:
        mPage = (mPage == 0u) ? static_cast<uint8_t>(pages - 1u)
                              : static_cast<uint8_t>(mPage - 1u);
        redraw();
        break;

    case Btn::L2:
        mPage = static_cast<uint8_t>((mPage + 1u) % pages);
        redraw();
        break;

    case Btn::R2:
        ScreenManager::instance().goTo(ScreenId::Main);
        break;

    default:
        break;
    }
}

void TrackSummaryScreen::onSummary(const ActivitySummary& /*summary*/)
{
    redraw();
}

void TrackSummaryScreen::redraw()
{
    const ActivitySummary& s = mModel.getSummary();

    for (uint8_t i = 0; i < kRowCount; ++i) {
        setRow(i, nullptr, nullptr);
    }

    if (!s.valid) {
        lv_label_set_text(mHeading, "No race yet");
        return;
    }

    char t[16];

    if (mPage == 0u) {
        // Overview: the totals brief 10.2 asks for.
        lv_label_set_text(mHeading, s.completed ? "Race complete" : "Ended early");

        uint8_t row = 0u;
        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.totalMs));
        setRow(row++, "Total", t);

        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.runsMs));
        setRow(row++, "Runs", t);

        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.stationsMs));
        setRow(row++, "Stations", t);

        if (s.roxzone) {
            Fmt::hms(t, sizeof(t), Fmt::msToSec(s.roxzoneMs));
            setRow(row++, "Roxzone", t);
        }

        // Two rows rather than one: "HR 146 avg 156 max" on a single line is
        // wider than the display is at that height.
        snprintf(t, sizeof(t), "%u bpm", static_cast<unsigned>(s.hrAvg));
        setRow(row++, "Avg HR", t);

        snprintf(t, sizeof(t), "%u bpm", static_cast<unsigned>(s.hrMax));
        setRow(row, "Max HR", t);
        return;
    }

    // Split pages.
    const uint8_t first = static_cast<uint8_t>((mPage - 1u) * kRowsPerPage);

    char buf[32];
    snprintf(buf, sizeof(buf), "Splits %u-%u of %u", static_cast<unsigned>(first + 1u),
             static_cast<unsigned>((first + kRowsPerPage < s.count) ? first + kRowsPerPage
                                                                    : s.count),
             static_cast<unsigned>(s.count));
    lv_label_set_text(mHeading, buf);

    for (uint8_t i = 0; i < kRowsPerPage; ++i) {
        const uint8_t index = static_cast<uint8_t>(first + i);
        if (index >= s.count || index >= Race::kMaxSegments) {
            break;  // no MMU: never read past the array
        }

        // The abbreviated name, not RaceModel::label(): the full form with its
        // work is twice the width this row has. The number keeps the row tied to
        // the lap of the same index in the FIT file.
        const uint8_t stationId = s.segments[index].stationId;
        const char *name = "SEGMENT";
        switch (static_cast<Race::SegmentType>(s.segments[index].type)) {
        case Race::SegmentType::Run:
            snprintf(buf, sizeof(buf), "%u RUN %u",
                     static_cast<unsigned>(index + 1u),
                     static_cast<unsigned>(s.segments[index].round));
            name = buf;
            break;
        case Race::SegmentType::RoxIn:
            snprintf(buf, sizeof(buf), "%u ROX IN", static_cast<unsigned>(index + 1u));
            name = buf;
            break;
        case Race::SegmentType::RoxOut:
            snprintf(buf, sizeof(buf), "%u ROX OUT", static_cast<unsigned>(index + 1u));
            name = buf;
            break;
        case Race::SegmentType::Station:
        default:
            snprintf(buf, sizeof(buf), "%u %s", static_cast<unsigned>(index + 1u),
                     (stationId >= 1u && stationId <= Race::kStationCount)
                             ? Race::kStations[stationId - 1u].brief
                             : "STATION");
            name = buf;
            break;
        }

        Fmt::shortTime(t, sizeof(t), Fmt::msToSec(s.segments[index].durationMs));
        setRow(i, name, t);
    }
}
