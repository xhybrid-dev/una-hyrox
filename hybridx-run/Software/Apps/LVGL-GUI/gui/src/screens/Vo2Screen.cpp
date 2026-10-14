/**
 ******************************************************************************
 * @file    Vo2Screen.cpp
 * @brief   HybridX Run: the VO2max page (see Vo2Screen.hpp).
 ******************************************************************************
 */

#include "gui/screens/Vo2Screen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

#include "Vo2Text.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

Vo2Screen::Vo2Screen(Model& model)
    : Screen(model)
{
}

void Vo2Screen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
}

uint8_t Vo2Screen::pageCount() const
{
    const uint8_t rows = mModel.getVo2Info().rowCount;
    return static_cast<uint8_t>(1 + (rows + kRowsPerPage - 1) / kRowsPerPage);
}

void Vo2Screen::onShow()
{
    mModel.resetIdleTimer();
    showPage(0);
}

void Vo2Screen::onVo2Info(const CustomMessage::Vo2Info& /*info*/)
{
    const uint8_t n = pageCount();
    showPage(mPageId < n ? mPageId : static_cast<uint8_t>(n - 1));
}

void Vo2Screen::showPage(uint8_t page)
{
    mTitle.reset();
    if (mPage) {
        lv_obj_delete(mPage);
        mPage = nullptr;
    }
    mPageId = page;
    mPage   = Theme::container(mRoot, 0, 0, 240, 240);
    if (page == 0) {
        buildOverview(mPage);
    } else {
        buildRuns(mPage, static_cast<uint8_t>(page - 1));
    }
    // Behind the scroll indicator and the button hints.
    lv_obj_move_background(mPage);
    mIndicator->setCount(pageCount());
    mIndicator->setActive(page);
    ScreenManager::logPool("vo2 page");
}

void Vo2Screen::buildOverview(lv_obj_t* f)
{
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
    namespace Text = RunVo2::Text;
    char line[40];
    char num[8];

    mTitle = std::make_unique<Widgets::Title>(f, "VO2MAX");
    Theme::label(f, F::Regular14, "ml/kg/min", 70, 50, 100, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Theme::label(f, F::SemiBold40, Text::formatX10(v.rollingX10, num, sizeof(num)), 60, 66, 120);

    if (v.runs == 0) {
        std::snprintf(line, sizeof(line), "No runs yet");
    } else if (Text::trend(v.rollingX10, v.beforeX10, line, sizeof(line))[0] == '\0') {
        std::snprintf(line, sizeof(line), "From 1 run");
    }
    Theme::label(f, F::Regular16, line, 20, 116, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Theme::hline(f, 25, 147, 190);

    const auto status = static_cast<RunVo2::ProfileStatus>(v.nowStatus);
    const char* why   = Text::profileReason(status);
    if (why[0] != '\0') {
        Theme::label(f, F::Regular16, why, 20, 156, 200, LV_TEXT_ALIGN_CENTER, Color::YELLOW_DARK);
        Theme::label(f, F::Regular16, "R1 to set it", 20, 180, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
        return;
    }
    const char* maxFrom  = Text::maxSourceLabel(static_cast<RunVo2::MaxHrSource>(v.nowMaxSource));
    const char* restFrom = Text::restSourceLabel(static_cast<RunVo2::RestHrSource>(v.nowRestSource));
    std::snprintf(line, sizeof(line), "Max HR %u (%s)", v.nowMaxHr, maxFrom);
    Theme::label(f, F::Regular16, line, 20, 156, 200);
    // "Rest", not "Resting": the longest ("Resting HR 100 (entered)") would
    // meet the round edge.
    std::snprintf(line, sizeof(line), "Rest HR %u (%s)", v.nowRestHr, restFrom);
    Theme::label(f, F::Regular16, line, 20, 180, 200);
}

void Vo2Screen::buildRuns(lv_obj_t* f, uint8_t listPage)
{
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
    mTitle = std::make_unique<Widgets::Title>(f, "LAST RUNS");
    char date[12];
    char num[8];
    const uint8_t first = static_cast<uint8_t>(listPage * kRowsPerPage);
    for (uint8_t r = 0; r < kRowsPerPage; ++r) {
        const uint8_t i = static_cast<uint8_t>(first + r);
        if (i >= v.rowCount || i >= CustomMessage::Vo2Info::kMaxRows) {
            break;
        }
        const int32_t y = 58 + r * 30;
        const CustomMessage::Vo2Info::Row& row = v.rows[i];
        Theme::label(f, F::Regular18, RunVo2::Text::shortDate(row.day, row.month, date, sizeof(date)),
                     44, y, 90, LV_TEXT_ALIGN_LEFT);
        Theme::label(f, F::SemiBold20, RunVo2::Text::formatX10(row.vo2x10, num, sizeof(num)),
                     130, y, 66, LV_TEXT_ALIGN_RIGHT);
    }
}

void Vo2Screen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L2:
            if (mPageId + 1 < pageCount()) {
                showPage(static_cast<uint8_t>(mPageId + 1));
            }
            break;
        case Btn::L1:
            if (mPageId > 0) {
                showPage(static_cast<uint8_t>(mPageId - 1));
            }
            break;
        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::MenuVo2Profile);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
    mModel.resetIdleTimer();
}
