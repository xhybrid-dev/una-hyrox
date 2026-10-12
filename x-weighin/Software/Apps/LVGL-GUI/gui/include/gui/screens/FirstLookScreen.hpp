/**
 ******************************************************************************
 * @file    FirstLookScreen.hpp
 * @brief   W0's one screen: what the watch knows about the athlete. The UNA
 *          profile's height and weight, the daily resting HR, and the age and
 *          sex from the phone settings. R2 exits.
 ******************************************************************************
 */

#ifndef FIRST_LOOK_SCREEN_HPP
#define FIRST_LOOK_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "Commands.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class FirstLookScreen : public ModelListener
{
public:
    explicit FirstLookScreen(Model& model);
    ~FirstLookScreen();

    FirstLookScreen(const FirstLookScreen&)            = delete;
    FirstLookScreen& operator=(const FirstLookScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

    // ModelListener
    void onFirstLook(const CustomMessage::FirstLookState& look) override;

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    void show(const CustomMessage::FirstLookState& look);

    Model&    mModel;
    lv_obj_t* mRoot    = nullptr;
    lv_obj_t* mTitle   = nullptr;
    lv_obj_t* mHeight  = nullptr;
    lv_obj_t* mWeight  = nullptr;
    lv_obj_t* mRestHr  = nullptr;
    lv_obj_t* mAge     = nullptr;
    lv_obj_t* mStatus  = nullptr;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // FIRST_LOOK_SCREEN_HPP
