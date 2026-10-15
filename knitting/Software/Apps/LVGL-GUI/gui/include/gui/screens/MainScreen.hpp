/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Knitting' one screen: the rows in this set (big), the set number,
 *          and the total rows (small).
 *          R1 counts a row, L1 starts the next set, L2 resets, R2 exits.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "RowCounter.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class MainScreen : public ModelListener
{
public:
    explicit MainScreen(Model& model);
    ~MainScreen();

    MainScreen(const MainScreen&)            = delete;
    MainScreen& operator=(const MainScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

    // ModelListener
    void onCounterUpdate(const Knitting::CounterState& state) override;

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    void show(const Knitting::CounterState& state);

    Model&    mModel;
    lv_obj_t* mRoot  = nullptr;
    lv_obj_t* mRows  = nullptr;
    lv_obj_t* mSet   = nullptr;
    lv_obj_t* mTotal = nullptr;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
