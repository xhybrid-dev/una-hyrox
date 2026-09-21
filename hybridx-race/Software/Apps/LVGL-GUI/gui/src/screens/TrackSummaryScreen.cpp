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

    mHeading = Theme::label(mRoot, F::Italic18, "", 0, 44, 240);
    for (uint8_t i = 0; i < kRowsPerPage; ++i) {
        mRows[i] = Theme::label(mRoot, F::Medium18, "", 12, 72 + i * 28, 216);
    }

    mTitle   = std::make_unique<Widgets::Title>(mRoot, "Summary");
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::NONE, Widgets::Buttons::AMBER);
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
    char buf[48];

    for (uint8_t i = 0; i < kRowsPerPage; ++i) {
        lv_label_set_text(mRows[i], "");
    }

    if (!s.valid) {
        lv_label_set_text(mHeading, "No race yet");
        return;
    }

    if (mPage == 0u) {
        // Overview: the totals brief 10.2 asks for.
        lv_label_set_text(mHeading, s.completed ? "Race complete" : "Ended early");

        char t[16];
        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.totalMs));
        snprintf(buf, sizeof(buf), "Total    %s", t);
        lv_label_set_text(mRows[0], buf);

        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.runsMs));
        snprintf(buf, sizeof(buf), "Runs     %s", t);
        lv_label_set_text(mRows[1], buf);

        Fmt::hms(t, sizeof(t), Fmt::msToSec(s.stationsMs));
        snprintf(buf, sizeof(buf), "Stations %s", t);
        lv_label_set_text(mRows[2], buf);

        uint8_t row = 3;
        if (s.roxzone) {
            Fmt::hms(t, sizeof(t), Fmt::msToSec(s.roxzoneMs));
            snprintf(buf, sizeof(buf), "Roxzone  %s", t);
            lv_label_set_text(mRows[row++], buf);
        }

        snprintf(buf, sizeof(buf), "HR  %u avg  %u max", static_cast<unsigned>(s.hrAvg),
                 static_cast<unsigned>(s.hrMax));
        lv_label_set_text(mRows[row], buf);
        return;
    }

    // Split pages.
    const uint8_t first = static_cast<uint8_t>((mPage - 1u) * kRowsPerPage);
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

        Race::SegmentDesc desc {};
        desc.type = static_cast<Race::SegmentType>(s.segments[index].type);
        desc.round = s.segments[index].round;
        desc.stationId = s.segments[index].stationId;

        char label[Race::kMaxLabelLen];
        Race::RaceModel::label(desc, label, sizeof(label));

        char t[16];
        Fmt::shortTime(t, sizeof(t), Fmt::msToSec(s.segments[index].durationMs));

        snprintf(buf, sizeof(buf), "%u %s", static_cast<unsigned>(index + 1u), t);
        lv_label_set_text(mRows[i], buf);
    }
}
