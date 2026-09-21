/**
 ******************************************************************************
 * @file    TrackStartConfirmScreen.hpp
 * @brief   "On your marks": R1 starts the race, R2 goes back. No idle exit.
 ******************************************************************************
 */

#ifndef TRACK_START_CONFIRM_SCREEN_HPP
#define TRACK_START_CONFIRM_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackStartConfirmScreen : public Screen
{
public:
    explicit TrackStartConfirmScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    lv_obj_t                         *mFormat   = nullptr;
    lv_obj_t                         *mSegments = nullptr;
    lv_obj_t                         *mRoxzone  = nullptr;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // TRACK_START_CONFIRM_SCREEN_HPP
