/**
 ******************************************************************************
 * @file    OfferScreen.hpp
 * @brief   A beaten spirit offers to run with you.
 *
 *   R1  yes    L2  no
 *
 * With a full pack, yes asks who leaves: L1 shows the next partner, R1 lets
 * that one go, L2 goes back to the offer.
 ******************************************************************************
 */

#ifndef TRAILQUEST_OFFER_SCREEN_HPP
#define TRAILQUEST_OFFER_SCREEN_HPP

#include "gui/Sprite.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class OfferScreen : public Screen
{
public:
    explicit OfferScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;

private:
    enum class Mode : uint8_t { Offer, WhoLeaves, Waiting };

    void setMode(Mode m);

    Sprite                     mSprite;
    lv_obj_t*                  mLine[2] = {};
    lv_obj_t*                  mHead    = nullptr;
    Ui::Corners                mCorners;
    TrailQuest::TextFit::Lines mText;
    Mode                       mMode     = Mode::Offer;
    uint8_t                    mSlot     = 0;
    uint32_t                   mLastFlip = 0;
    uint8_t                    mFrame    = 0;
};

#endif // TRAILQUEST_OFFER_SCREEN_HPP
