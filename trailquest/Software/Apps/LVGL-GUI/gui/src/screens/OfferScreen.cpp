/**
 ******************************************************************************
 * @file    OfferScreen.cpp
 * @brief   A beaten spirit offers to run with you (see OfferScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/OfferScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/Art.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

namespace
{
// The spirit stands in the middle; its two lines are under it.
constexpr int32_t kSpriteY = 74;
constexpr int32_t kLineRow = 124;   // two lines, clear of the bottom words (row 160)
} // namespace

void OfferScreen::build()
{
    Draw::box(mRoot, 0, 94, 240, 28, 0x008000);
    mSprite.create(mRoot, 96, kSpriteY, TRAILQUEST_SPRITE_STITCH, SpriteBuffers::small(0), SpriteBuffers::kSmall);
    mHead    = Ui::rowLabel(mRoot, Layout::kHeadRow, "", Ui::Ink::kGold);
    mLine[0] = Ui::rowLabel(mRoot, kLineRow, mText.text[0]);
    mLine[1] = Ui::rowLabel(mRoot, kLineRow + TextFit::kGlyph, mText.text[1]);
    mCorners.create(mRoot);
}

void OfferScreen::onShow()
{
    setMode(Mode::Offer);
}

void OfferScreen::setMode(Mode m)
{
    mMode = m;
    const RunState& s = mModel.world().state;
    switch (m) {
        case Mode::Offer:
            mSprite.show(Art::spriteFor(s.offer));
            Narrate::offer(mText, s.offer, false);
            Ui::setText(mHead, "BEATEN", Ui::Ink::kGold);
            Ui::setText(mLine[0], mText.text[0], Ui::affinityInk(static_cast<uint8_t>(speciesDef(s.offer).affinity)));
            Ui::setText(mLine[1], mText.text[1], Ui::Ink::kText);
            mCorners.set(nullptr, Label::kYes, Label::kNo, nullptr);
            setArcs(Buttons::NONE, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
            break;
        case Mode::WhoLeaves: {
            const SpeciesId p = s.partners[mSlot < kMaxPartners ? mSlot : 0];
            mSprite.show(Art::spriteFor(p));
            Narrate::offer(mText, p, true);
            Ui::setText(mHead, "PACK FULL", Ui::Ink::kRed);
            Ui::setText(mLine[0], mText.text[0], Ui::affinityInk(static_cast<uint8_t>(speciesDef(p).affinity)));
            Ui::setText(mLine[1], mText.text[1], Ui::Ink::kText);
            mCorners.set(Label::kNext, Label::kYes, Label::kBack, nullptr);
            setArcs(Buttons::AMBER, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
        } break;
        case Mode::Waiting:
            mCorners.set(nullptr, nullptr, nullptr, nullptr);
            setArcs(Buttons::NONE, Buttons::NONE, Buttons::NONE, Buttons::RED);
            break;
    }
}

void OfferScreen::onKey(uint8_t code)
{
    const RunState& s = mModel.world().state;
    switch (mMode) {
        case Mode::Offer:
            if (code == Btn::R1) {
                if (s.partnerCount < kMaxPartners) {
                    setMode(Mode::Waiting);
                    mModel.join(true, kNone);
                } else {
                    mSlot = 0;
                    setMode(Mode::WhoLeaves);
                }
            } else if (code == Btn::L2) {
                setMode(Mode::Waiting);
                mModel.join(false, kNone);
            }
            break;
        case Mode::WhoLeaves:
            if (code == Btn::L1) {
                mSlot = static_cast<uint8_t>((mSlot + 1u) % kMaxPartners);
                setMode(Mode::WhoLeaves);
            } else if (code == Btn::R1) {
                setMode(Mode::Waiting);
                mModel.join(true, mSlot);
            } else if (code == Btn::L2) {
                setMode(Mode::Offer);
            }
            break;
        default:
            break;
    }
}

void OfferScreen::onWorld()
{
    if (mMode != Mode::Waiting || mModel.waiting()) {
        return;
    }
    const WorldView& w = mModel.world();
    if (!mModel.lost() && w.cause == ViewCause::Joined) {
        mModel.playFx(FxId::Win);
    }
    if (mModel.lost() || w.cause == ViewCause::Refused) {
        setMode(Mode::Offer);
        return;
    }
    ScreenManager::instance().goTo(ScreenManager::screenFor(w));
}

void OfferScreen::onFrame(uint32_t nowMs)
{
    if (nowMs - mLastFlip >= 500) {
        mLastFlip = nowMs;
        mFrame ^= 1u;
        mSprite.setFrame(mFrame);
    }
}
