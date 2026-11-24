/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   The Energy Probe's one screen, in six pages: Now, Nights, Tonight,
 *          Night window, Daytime and Sensor check (x-energy/docs/PROBE.md).
 *
 * L1 and L2 turn the pages, R1 is the page's action, R2 closes the screen.
 * Closing the screen does not stop the logging.
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
    static constexpr uint8_t kLines = 6;
    static constexpr uint8_t kPages = 6;

    enum Page : uint8_t { NOW = 0, NIGHTS, TONIGHT, WINDOW, DAYTIME, CHECK };

    explicit MainScreen(Model& model);
    ~MainScreen();

    MainScreen(const MainScreen&)            = delete;
    MainScreen& operator=(const MainScreen&) = delete;

    lv_obj_t* root() const { return mRoot; }

    void onChanged() override;

private:
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    void draw();
    void listLayout();
    void bigLayout();
    void line(uint8_t i, const char* text, uint32_t color);
    void drawNow();
    void drawNights();
    void drawTonight();
    void drawWindow();
    void drawDaytime();
    void drawCheck();

    Model&    mModel;
    lv_obj_t* mRoot  = nullptr;
    lv_obj_t* mTitle = nullptr;
    lv_obj_t* mPageNo = nullptr;
    lv_obj_t* mBig   = nullptr;
    lv_obj_t* mFoot  = nullptr;
    lv_obj_t* mLine[kLines] {};
    uint8_t   mPage  = NOW;
    uint8_t   mCheckBlock = 0;
    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
