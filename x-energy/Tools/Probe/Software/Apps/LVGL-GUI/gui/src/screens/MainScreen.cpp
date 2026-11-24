/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   The Energy Probe's screen (see the header).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "CivilDate.hpp"
#include "ProbeMode.hpp"
#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using namespace EnergyProbe;

namespace
{
// Rows narrower towards the top and bottom of the round face.
constexpr int32_t kListY    = 66;
constexpr int32_t kListStep = 22;
constexpr int32_t kLineX[MainScreen::kLines] = {26, 20, 18, 18, 20, 26};

const char* const kTitles[MainScreen::kPages] = {"NOW", "NIGHTS", "TONIGHT", "NIGHT WINDOW", "DAYTIME",
                                                 "SENSOR CHECK"};

void dateText(char* out, size_t cap, int32_t day)
{
    const CivilDate c = civilFromDays(day);
    std::snprintf(out, cap, "%u %s", static_cast<unsigned>(c.day), monthShort(c.month));
}

/// x100 as "0.42"; "?" when unknown.
void hundredths(char* out, size_t cap, int32_t v)
{
    if (v == INT32_MIN) {
        std::snprintf(out, cap, "?");
        return;
    }
    const bool     neg = v < 0;
    const uint32_t a   = neg ? static_cast<uint32_t>(-static_cast<int64_t>(v)) : static_cast<uint32_t>(v);
    std::snprintf(out, cap, "%s%lu.%02lu", neg ? "-" : "", static_cast<unsigned long>(a / 100u),
                  static_cast<unsigned long>(a % 100u));
}
} // namespace

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    mTitle  = Draw::label(mRoot, &poppins_semibold_20, "", 20, 16, 200, LV_TEXT_ALIGN_CENTER, Color::SKY);
    mPageNo = Draw::label(mRoot, &poppins_regular_14, "", 70, 42, 100, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mBig    = Draw::label(mRoot, &poppins_semibold_25, "", 20, 90, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    for (uint8_t i = 0; i < kLines; ++i) {
        mLine[i] = Draw::label(mRoot, &poppins_regular_14, "", kLineX[i], kListY + i * kListStep,
                               240 - 2 * kLineX[i], LV_TEXT_ALIGN_CENTER, Color::WHITE);
    }
    mFoot = Draw::label(mRoot, &poppins_regular_14, "", 50, 202, 140, LV_TEXT_ALIGN_CENTER, Color::YELLOW_DARK);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE, SDK::LVGL::Buttons::WHITE,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::WHITE);

    mModel.bind(this);
    draw();
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const CustomMessage::StatusData& s = mModel.status();
    switch (code) {
        case Btn::L1:
            mPage = static_cast<uint8_t>((mPage + kPages - 1) % kPages);
            mCheckBlock = 0;
            draw();
            break;
        case Btn::L2:
            mPage = static_cast<uint8_t>((mPage + 1) % kPages);
            mCheckBlock = 0;
            draw();
            break;
        case Btn::R1:
            if (!s.valid && mPage != CHECK) {
                break;
            }
            switch (mPage) {
                case TONIGHT:
                    mModel.setOverride(static_cast<uint8_t>((s.overrideMode + 1u) % kOverrideCount));
                    break;
                case WINDOW:
                    mModel.setWindow(static_cast<uint8_t>((s.window + 1u) % kWindowCount));
                    break;
                case DAYTIME:
                    mModel.setDayMode(s.dayMode == static_cast<uint8_t>(Mode::Off)
                                          ? static_cast<uint8_t>(Mode::Full)
                                          : static_cast<uint8_t>(Mode::Off));
                    break;
                case CHECK:
                    if (s.checkRunning) {
                        break;
                    }
                    if (mModel.checkDone() && (mCheckBlock + 1u) * kLines < mModel.checkCount()) {
                        ++mCheckBlock;
                        draw();
                    } else {
                        mCheckBlock = 0;
                        mModel.startCheck();
                        draw();
                    }
                    break;
                default:
                    break;
            }
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void MainScreen::onChanged()
{
    draw();
}

void MainScreen::listLayout()
{
    Draw::setHidden(mBig, true);
    for (uint8_t i = 0; i < kLines; ++i) {
        lv_obj_set_y(mLine[i], kListY + i * kListStep);
    }
}

void MainScreen::bigLayout()
{
    // A line above the big value, three below it.
    Draw::setHidden(mBig, false);
    lv_obj_set_y(mLine[0], 64);
    lv_obj_set_y(mLine[1], 128);
    lv_obj_set_y(mLine[2], 150);
    lv_obj_set_y(mLine[3], 172);
    lv_obj_set_y(mLine[4], 194);   // empty in this layout
    lv_obj_set_y(mLine[5], 194);
}

void MainScreen::line(uint8_t i, const char* text, uint32_t color)
{
    if (i >= kLines) {
        return;
    }
    lv_label_set_text(mLine[i], text);
    lv_obj_set_style_text_color(mLine[i], lv_color_hex(color), 0);
}

void MainScreen::draw()
{
    lv_label_set_text(mTitle, kTitles[mPage % kPages]);
    char t[16];
    std::snprintf(t, sizeof(t), "%u of %u", static_cast<unsigned>(mPage + 1), static_cast<unsigned>(kPages));
    lv_label_set_text(mPageNo, t);
    for (uint8_t i = 0; i < kLines; ++i) {
        line(i, "", Color::WHITE);
    }
    lv_label_set_text(mFoot, "");
    lv_label_set_text(mBig, "");

    if (!mModel.status().valid && mPage != CHECK) {
        listLayout();
        line(1, "Waiting for the logger", Color::GRAY);
        return;
    }
    switch (mPage) {
        case NOW:     drawNow(); break;
        case NIGHTS:  drawNights(); break;
        case TONIGHT: drawTonight(); break;
        case WINDOW:  drawWindow(); break;
        case DAYTIME: drawDaytime(); break;
        case CHECK:   drawCheck(); break;
        default: break;
    }
}

void MainScreen::drawNow()
{
    listLayout();
    const CustomMessage::StatusData& s = mModel.status();
    char t[48];

    std::snprintf(t, sizeof(t), "Now: %s, %s", modeName(modeFrom(s.modeNow)), s.night ? "night" : "day");
    line(0, t, Color::WHITE);

    const Window w = window(s.window);
    if (s.night) {
        std::snprintf(t, sizeof(t), "Until %02u:00", static_cast<unsigned>(w.endHour));
    } else {
        std::snprintf(t, sizeof(t), "From %02u:00: %s", static_cast<unsigned>(w.startHour),
                      modeName(modeFrom(s.tonightMode)));
    }
    line(1, t, Color::GRAY);

    if (s.battPctX100 >= 0) {
        char pct[16];
        hundredths(pct, sizeof(pct), s.battPctX100);
        std::snprintf(t, sizeof(t), "Battery %s%%%s", pct, s.charging == 1 ? ", charging" : "");
    } else {
        std::snprintf(t, sizeof(t), "Battery: no reading yet");
    }
    line(2, t, Color::WHITE);

    const char* wrist = s.touch == 1 ? ", on wrist" : (s.touch == 0 ? ", off wrist" : "");
    if (s.lastBpm > 0) {
        std::snprintf(t, sizeof(t), "HR %u%s", static_cast<unsigned>(s.lastBpm), wrist);
    } else {
        std::snprintf(t, sizeof(t), "HR --%s", wrist);
    }
    line(3, t, Color::WHITE);

    std::snprintf(t, sizeof(t), "%lu rows, %lu KB", static_cast<unsigned long>(s.rows),
                  static_cast<unsigned long>(s.kbWritten));
    line(4, t, Color::GRAY);

    if (!s.timeValid) {
        line(5, "Clock not set: sync it", Color::RED);
    } else if (s.writeErrors > 0) {
        std::snprintf(t, sizeof(t), "Write errors: %lu", static_cast<unsigned long>(s.writeErrors));
        line(5, t, Color::RED);
    } else if (s.groupsFailed != 0) {
        std::snprintf(t, sizeof(t), "Sensors failed: 0x%lx", static_cast<unsigned long>(s.groupsFailed));
        line(5, t, Color::YELLOW_DARK);
    } else {
        std::snprintf(t, sizeof(t), "Logging, start %lu", static_cast<unsigned long>(s.starts));
        line(5, t, Color::GREEN);
    }
}

void MainScreen::drawNights()
{
    listLayout();
    const CustomMessage::NightsData& n = mModel.nights();
    if (n.count == 0) {
        line(1, "No night finished yet.", Color::WHITE);
        char t[40];
        std::snprintf(t, sizeof(t), "The first ends %02u:00.",
                      static_cast<unsigned>(window(mModel.status().window).endHour));
        line(2, t, Color::GRAY);
        line(4, "Drain in % an hour", Color::GRAY);
        return;
    }
    line(0, "Night, mode, %/h, mA", Color::GRAY);
    for (uint8_t i = 0; i < n.count && i < kLines - 1; ++i) {
        const NightBrief& b = n.nights[i];
        char date[12];
        char drain[12];
        char ma[12];
        dateText(date, sizeof(date), b.day);
        hundredths(drain, sizeof(drain), b.drainX100);
        // The fuel gauge's mean current if it reported one, else the one from the mAh drop.
        int32_t cur = b.gaugeMaX100 != INT32_MIN ? b.gaugeMaX100 : b.capMaX100;
        if (cur != INT32_MIN && cur < 0) {
            cur = -cur;   // discharge may be reported as negative
        }
        hundredths(ma, sizeof(ma), cur);
        char t[48];
        std::snprintf(t, sizeof(t), "%s %s %s %s%s", date, modeName(modeFrom(b.mode)), drain, ma,
                      (b.flags & kFlagCharged) ? " C" : ((b.flags & (kFlagRestarted | kFlagPartial | kFlagModeChanged)) ? " *" : ""));
        line(static_cast<uint8_t>(i + 1), t, (b.flags & kFlagCharged) ? Color::GRAY : Color::WHITE);
    }
}

void MainScreen::drawTonight()
{
    bigLayout();
    const CustomMessage::StatusData& s = mModel.status();
    char date[12];
    dateText(date, sizeof(date), s.tonightDay);
    char t[40];
    std::snprintf(t, sizeof(t), "%s, %s", s.night ? "This night" : "Tonight", date);
    line(0, t, Color::GRAY);
    const Mode m = modeFrom(s.tonightMode);
    lv_label_set_text(mBig, modeName(m));
    line(1, modeHint(m), Color::WHITE);
    line(2, s.overrideMode != kOverrideAuto ? "Set by you" : "From the rotation", Color::GRAY);
    lv_label_set_text(mFoot, "R1: change");
}

void MainScreen::drawWindow()
{
    bigLayout();
    const Window w = window(mModel.status().window);
    line(0, "Nights run", Color::GRAY);
    char t[16];
    std::snprintf(t, sizeof(t), "%02u:00-%02u:00", static_cast<unsigned>(w.startHour), static_cast<unsigned>(w.endHour));
    lv_label_set_text(mBig, t);
    line(1, "The drain is worked", Color::WHITE);
    line(2, "out over this window", Color::WHITE);
    lv_label_set_text(mFoot, "R1: change");
}

void MainScreen::drawDaytime()
{
    bigLayout();
    const bool off = mModel.status().dayMode == static_cast<uint8_t>(Mode::Off);
    line(0, "Daytime records", Color::GRAY);
    lv_label_set_text(mBig, off ? "Off" : "Full");
    line(1, off ? "the battery only" : "every sensor", Color::WHITE);
    line(2, off ? "No naps or rest seen" : "Naps, rest and moves", Color::GRAY);
    lv_label_set_text(mFoot, "R1: change");
}

void MainScreen::drawCheck()
{
    listLayout();
    const CustomMessage::StatusData& s = mModel.status();
    char t[40];
    if (s.checkRunning || (!mModel.checkDone() && mModel.checkSecsLeft() > 0)) {
        line(1, "Listening to every", Color::WHITE);
        line(2, "sensor the SDK lists", Color::WHITE);
        std::snprintf(t, sizeof(t), "%u s to go", static_cast<unsigned>(mModel.checkSecsLeft()));
        line(4, t, Color::YELLOW_DARK);
        return;
    }
    if (mModel.checkDone() && mModel.checkCount() > 0) {
        for (uint8_t i = 0; i < kLines; ++i) {
            const uint8_t k = static_cast<uint8_t>(mCheckBlock * kLines + i);
            if (k < mModel.checkCount()) {
                line(i, mModel.checkLine(k), Color::WHITE);
            }
        }
        const bool more = (mCheckBlock + 1u) * kLines < mModel.checkCount();
        lv_label_set_text(mFoot, more ? "R1: more" : "R1: run again");
        return;
    }
    line(0, "Wear the watch and", Color::WHITE);
    line(1, "sit still: 2 minutes.", Color::WHITE);
    line(2, "It tries every sensor", Color::GRAY);
    line(3, "and saves check_*.csv", Color::GRAY);
    lv_label_set_text(mFoot, "R1: start");
}
