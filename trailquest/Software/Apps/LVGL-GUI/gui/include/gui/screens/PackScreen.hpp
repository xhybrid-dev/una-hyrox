/**
 ******************************************************************************
 * @file    PackScreen.hpp
 * @brief   The runner, the pack and the bag, a page each.
 *
 *   R1  next page    L2  back to the map
 ******************************************************************************
 */

#ifndef TRAILQUEST_PACK_SCREEN_HPP
#define TRAILQUEST_PACK_SCREEN_HPP

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class PackScreen : public Screen
{
public:
    explicit PackScreen(Model& model) : Screen(model) {}

    void onShow() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;

private:
    void showPage();

    lv_obj_t*   mHead = nullptr;
    Ui::Card    mCard;
    Ui::Corners mCorners;
    char        mHeadText[16] = {};
    uint8_t     mPage = 0;
};

#endif // TRAILQUEST_PACK_SCREEN_HPP
