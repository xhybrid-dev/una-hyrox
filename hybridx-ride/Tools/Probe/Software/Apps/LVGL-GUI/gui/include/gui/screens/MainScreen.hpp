/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   The bike probe's one screen: the next request, the log, the strap.
 *
 * R1 asks the kernel for the next accessory kinds (ProbeText's steps). L1 and
 * L2 scroll the log (newer, older). R2 exits.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class MainScreen : public ModelListener
{
public:
    static constexpr uint8_t kShown = 6;   ///< log lines on screen

    explicit MainScreen(Model& model);
    ~MainScreen();

    MainScreen(const MainScreen&)            = delete;
    MainScreen& operator=(const MainScreen&) = delete;

    lv_obj_t* root() const { return mRoot; }

    void onLog() override;
    void onState() override;

private:
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    void drawLog();

    Model&    mModel;
    lv_obj_t* mRoot  = nullptr;
    lv_obj_t* mNext  = nullptr;   ///< "R1: bit 1 (cadence?)"
    lv_obj_t* mHr    = nullptr;   ///< the heart rate's source and the strap's bpm
    lv_obj_t* mLog[kShown] {};
    uint8_t   mScroll = 0;        ///< lines scrolled back from the newest
    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
