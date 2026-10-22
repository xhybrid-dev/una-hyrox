/**
 ******************************************************************************
 * @file    SessionScreen.cpp
 * @brief   X Sauna's interim session screen (see the header).
 ******************************************************************************
 */

#include "gui/screens/SessionScreen.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "SessionModel.hpp"
#include "SessionText.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using Btn = SDK::LVGL::Buttons;

namespace
{
constexpr uint32_t kTickMs = 40;

/// Text lines, as the Menu, Confirm and Done views lay them out. Positions keep
/// clear of the bezel arcs at the corners, and of the round face's edge.
constexpr int32_t kLineY[6] = {50, 78, 106, 134, 162, 188};

uint32_t phaseColor(Sauna::Phase p)
{
    switch (p) {
        case Sauna::Phase::Sauna:  return Color::ORANGE_DARK;
        case Sauna::Phase::Plunge: return Color::SKY;
        default:                   return Color::GREEN;
    }
}
} // namespace

SessionScreen::SessionScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &SessionScreen::keyEventCb, LV_EVENT_KEY, this);

    mTitle = Draw::label(mRoot, &poppins_semibold_20, "X SAUNA", 40, 20, 160,
                         LV_TEXT_ALIGN_CENTER, Color::YELLOW_DARK);
    mBig   = Draw::label(mRoot, &poppins_light_60, "", 20, 44, 200);
    for (int i = 0; i < 6; ++i) {
        mL[i] = Draw::label(mRoot, &poppins_regular_18, "", 24, kLineY[i], 192,
                            LV_TEXT_ALIGN_CENTER, Color::WHITE);
    }

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);

    bind(&mModel);
    mModel.bind(this);

    mSnap = mModel.snapshot();
    const bool active = mSnap.state == static_cast<uint8_t>(Sauna::RunState::Running) ||
                        mSnap.state == static_cast<uint8_t>(Sauna::RunState::Paused);
    mView = active ? View::Live : View::Menu;
    draw();
}

SessionScreen::~SessionScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void SessionScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<SessionScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void SessionScreen::tickCb(lv_timer_t* t)
{
    static_cast<SessionScreen*>(lv_timer_get_user_data(t))->tick();
}

void SessionScreen::onKey(uint8_t code)
{
    namespace B = SDK::GUI::Button;
    const uint32_t now = lv_tick_get();

    // R2 in the live view is a hold; the other views use the plain click.
    if (mView == View::Live) {
        switch (code) {
            case B::R2_PRESS:
                mHasHint = false;
                mStop.press(now);
                break;
            case B::R2_RELEASE:
                if (mStop.release(now)) {
                    hint("Hold R2 to stop");
                }
                break;
            case B::R2:
                if (mStop.takeClick(now)) {
                    hint("Hold R2 to stop");
                }
                break;
            case B::R1:
                if (mSnap.state == static_cast<uint8_t>(Sauna::RunState::Running)) {
                    mModel.lap();
                }
                break;
            case B::L1:
                if (mSnap.state == static_cast<uint8_t>(Sauna::RunState::Paused)) {
                    mModel.resume();
                } else {
                    mModel.pause();
                }
                break;
            default:
                return;
        }
        draw();
        updateTimer();
        return;
    }

    switch (mView) {
        case View::Menu:
            if (code == B::R1) {
                mModel.startSession(0, mRest ? 1 : 0);
            } else if (code == B::L1) {
                mModel.startSession(1, 0);
            } else if (code == B::L2) {
                mRest = !mRest;
                draw();
            } else if (code == B::R2) {
                mModel.exitApp();
            }
            break;

        case View::Confirm:
            if (code == B::R1) {
                mModel.stop(true);
            } else if (code == B::L1) {
                mModel.stop(false);
            } else if (code == B::R2) {
                setView(View::Live);
            }
            break;

        case View::Done:
            if (code == B::R1) {
                setView(View::Menu);
            } else if (code == B::R2) {
                mModel.exitApp();
            }
            break;

        default:
            break;
    }
}

void SessionScreen::tick()
{
    const uint32_t now = lv_tick_get();
    if (mStop.poll(now)) {
        setView(View::Confirm);
    }
    if (mHasHint && now - mHintAt >= kHintMs) {
        mHasHint = false;
        draw();
    }
    updateTimer();
}

void SessionScreen::updateTimer()
{
    const bool needed = mStop.pending() || mHasHint;
    if (needed && !mTimer) {
        mTimer = lv_timer_create(&SessionScreen::tickCb, kTickMs, this);
    } else if (!needed && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void SessionScreen::onSuspend()
{
    // The release cannot reach this screen while the GUI is suspended, so a
    // hold left running could complete by itself on resume. Drop it.
    mStop.cancel();
    mHasHint = false;
    updateTimer();
}

void SessionScreen::hint(const char* s)
{
    std::snprintf(mHint, sizeof(mHint), "%s", s);
    mHasHint = true;
    mHintAt  = lv_tick_get();
}

void SessionScreen::setView(View v)
{
    mView = v;
    mStop.cancel();
    mHasHint = false;
    draw();
    updateTimer();
}

void SessionScreen::onSessionUpdate(const Sauna::SessionSnapshot& snapshot)
{
    mSnap = snapshot;
    const bool active = snapshot.state == static_cast<uint8_t>(Sauna::RunState::Running) ||
                        snapshot.state == static_cast<uint8_t>(Sauna::RunState::Paused);
    if (active) {
        if (mView == View::Menu || mView == View::Done) {
            mView = View::Live;
        }
    } else if (mView == View::Live || mView == View::Confirm) {
        // The service ended it (the screen was away, say): the Saved message
        // brings the summary; until then show the menu.
        mView = View::Menu;
    }
    draw();
}

void SessionScreen::onSessionSaved(const Sauna::SavedSession& saved)
{
    mSaved = saved;
    setView(View::Done);
}

void SessionScreen::text(lv_obj_t* label, const char* s, uint32_t color, const lv_font_t* font)
{
    lv_label_set_text(label, s);
    lv_obj_set_style_text_color(label, Draw::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
}

void SessionScreen::draw()
{
    for (int i = 0; i < 6; ++i) {
        text(mL[i], "", Color::WHITE, &poppins_regular_18);
        lv_obj_set_y(mL[i], kLineY[i]);
    }
    text(mBig, "", Color::WHITE, &poppins_light_60);

    switch (mView) {
        case View::Menu:    drawMenu(); break;
        case View::Live:    drawLive(); break;
        case View::Confirm: drawConfirm(); break;
        case View::Done:    drawDone(); break;
    }
}

void SessionScreen::drawMenu()
{
    text(mTitle, "X SAUNA", Color::YELLOW_DARK, &poppins_semibold_20);
    text(mL[0], "R1  Sauna + plunge", Color::WHITE, &poppins_regular_18);
    text(mL[1], "L1  Just sauna", Color::WHITE, &poppins_regular_18);
    text(mL[2], mRest ? "L2  Rest: on" : "L2  Rest: off", Color::GRAY, &poppins_regular_18);
    // Below the top bezel arcs, which end about y = 88.
    lv_obj_set_y(mL[0], 96);
    lv_obj_set_y(mL[1], 126);
    lv_obj_set_y(mL[2], 156);
    mButtons->set(Btn::WHITE, Btn::WHITE, Btn::AMBER, Btn::WHITE);
}

void SessionScreen::drawLive()
{
    const bool paused = mSnap.state == static_cast<uint8_t>(Sauna::RunState::Paused);
    const Sauna::Phase phase = static_cast<Sauna::Phase>(mSnap.phase);
    char buf[48];
    char a[16];

    text(mTitle, paused ? "PAUSED" : Sauna::phaseName(phase), paused ? Color::GRAY : phaseColor(phase),
         &poppins_semibold_20);

    Sauna::formatClock(a, sizeof(a), mSnap.lapMs);
    text(mBig, a, Color::WHITE, &poppins_light_60);

    Sauna::formatBpm(a, sizeof(a), mSnap.hr);
    std::snprintf(buf, sizeof(buf), "HR %s", a);
    text(mL[2], buf, Color::GREEN, &poppins_semibold_25);
    lv_obj_set_y(mL[2], 116);

    char total[16];
    Sauna::formatClock(total, sizeof(total), mSnap.activeMs);
    std::snprintf(buf, sizeof(buf), "total %s  round %u", total, static_cast<unsigned>(mSnap.round));
    text(mL[3], buf, Color::GRAY, &poppins_regular_14);
    lv_obj_set_y(mL[3], 152);

    if (mHasHint) {
        text(mL[4], mHint, Color::YELLOW_DARK, &poppins_regular_14);
    } else if (!mSnap.logOk) {
        text(mL[4], "NO LOG", Color::RED, &poppins_regular_14);
    } else if (mSnap.laps > 1 && mSnap.prevRecovery != 0) {
        Sauna::Recovery r;
        r.state = static_cast<Sauna::Recovery::State>(mSnap.prevRecovery);
        r.drop  = mSnap.prevDrop;
        Sauna::formatRecovery(a, sizeof(a), r);
        std::snprintf(buf, sizeof(buf), "last recovery %s", a);
        text(mL[4], buf, Color::GRAY, &poppins_regular_14);
    }
    lv_obj_set_y(mL[4], 174);

    if (mStop.pending()) {
        text(mL[5], "Hold R2 to stop", Color::YELLOW_DARK, &poppins_regular_14);
    }
    lv_obj_set_y(mL[5], 196);
    mButtons->set(Btn::WHITE, Btn::NONE, paused ? Btn::NONE : Btn::AMBER, Btn::RED);
}

void SessionScreen::drawConfirm()
{
    text(mTitle, "END SESSION?", Color::YELLOW_DARK, &poppins_semibold_20);
    text(mL[1], "R1  Save", Color::GREEN, &poppins_regular_18);
    text(mL[2], "L1  Discard", Color::RED, &poppins_regular_18);
    text(mL[3], "R2  Back", Color::WHITE, &poppins_regular_18);
    mButtons->set(Btn::RED, Btn::NONE, Btn::GREEN, Btn::WHITE);
}

void SessionScreen::drawDone()
{
    const Sauna::SessionSummary& s = mSaved.summary;
    char buf[48];
    char a[16];

    text(mTitle, mSaved.saved ? "SAVED" : "NOT SAVED", mSaved.saved ? Color::GREEN : Color::RED,
         &poppins_semibold_20);

    std::snprintf(buf, sizeof(buf), "%u round%s", static_cast<unsigned>(s.rounds), s.rounds == 1 ? "" : "s");
    text(mL[0], buf, Color::WHITE, &poppins_regular_18);

    Sauna::formatClock(a, sizeof(a), s.saunaMs);
    std::snprintf(buf, sizeof(buf), "sauna %s", a);
    text(mL[1], buf, Color::ORANGE_DARK, &poppins_regular_18);

    Sauna::formatClock(a, sizeof(a), s.plungeMs);
    std::snprintf(buf, sizeof(buf), "plunge %s", a);
    text(mL[2], buf, Color::SKY, &poppins_regular_18);

    Sauna::formatClock(a, sizeof(a), s.restMs);
    std::snprintf(buf, sizeof(buf), "rest %s", a);
    text(mL[3], buf, Color::GREEN, &poppins_regular_18);

    char x[8];
    char y[8];
    Sauna::formatBpm(x, sizeof(x), s.hrMax);
    Sauna::formatBpm(y, sizeof(y), s.hrAvg);
    std::snprintf(buf, sizeof(buf), "HR max %s  avg %s", x, y);
    text(mL[4], buf, Color::GRAY, &poppins_regular_14);

    if (s.hasRecovery) {
        std::snprintf(buf, sizeof(buf), "best recovery -%d", static_cast<int>(s.bestDrop));
    } else {
        std::snprintf(buf, sizeof(buf), "best recovery --");
    }
    text(mL[5], buf, Color::GRAY, &poppins_regular_14);
    lv_obj_set_y(mL[4], 166);
    lv_obj_set_y(mL[5], 188);
    mButtons->set(Btn::NONE, Btn::NONE, Btn::AMBER, Btn::WHITE);
}
