/**
 ******************************************************************************
 * @file    PackScreen.cpp
 * @brief   The runner, the pack and the bag (see PackScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/PackScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"

#include "Layout.hpp"
#include "Narrate.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace Btn   = SDK::GUI::Button;
namespace Label = TrailQuest::Narrate::Label;
using SDK::LVGL::Buttons;
using namespace TrailQuest;

namespace
{
constexpr uint8_t kPages = 3;
} // namespace

void PackScreen::build()
{
    mHead = Ui::rowLabel(mRoot, Layout::kHeadRow, mHeadText, Ui::Ink::kGold);
    mCard.create(mRoot);
    mCorners.create(mRoot);
}

void PackScreen::onShow()
{
    mCorners.set(nullptr, Label::kNext, Label::kBack, nullptr);
    setArcs(Buttons::NONE, Buttons::WHITE, Buttons::GREEN, Buttons::RED);
    showPage();
}

void PackScreen::showPage()
{
    const RunState& s = mModel.world().state;
    const Stats     st = runnerMaxWithPartners(s);
    TextFit::Lines  lines;
    char            line[24];
    uint32_t        colors[TextFit::kMaxLines];
    for (auto& c : colors) {
        c = Ui::Ink::kText;
    }

    switch (mPage) {
        case 0:
            std::snprintf(mHeadText, sizeof(mHeadText), "LEVEL %u", static_cast<unsigned>(s.level));
            if (s.level < kMaxLevel) {
                std::snprintf(line, sizeof(line), "XP %u/%u", static_cast<unsigned>(s.xp),
                              static_cast<unsigned>(xpToNext(s.level)));
            } else {
                std::snprintf(line, sizeof(line), "XP: top");
            }
            TextFit::addLine(lines, line, Layout::kCardCols);
            // "Stamina 34/40"; past 99 it would not fit the row, so shorter.
            if (std::snprintf(line, sizeof(line), "Stamina %u/%u", static_cast<unsigned>(s.stamina),
                              static_cast<unsigned>(st.stamina)) > Layout::kCardCols) {
                std::snprintf(line, sizeof(line), "Sta %u/%u", static_cast<unsigned>(s.stamina),
                              static_cast<unsigned>(st.stamina));
            }
            TextFit::addLine(lines, line, Layout::kCardCols);
            std::snprintf(line, sizeof(line), "Pace %u", static_cast<unsigned>(st.pace));
            colors[lines.count] = Ui::Ink::kPace;
            TextFit::addLine(lines, line, Layout::kCardCols);
            std::snprintf(line, sizeof(line), "Form %u", static_cast<unsigned>(st.form));
            colors[lines.count] = Ui::Ink::kForm;
            TextFit::addLine(lines, line, Layout::kCardCols);
            std::snprintf(line, sizeof(line), "Grit %u", static_cast<unsigned>(st.grit));
            colors[lines.count] = Ui::Ink::kGrit;
            TextFit::addLine(lines, line, Layout::kCardCols);
            break;
        case 1:
            std::snprintf(mHeadText, sizeof(mHeadText), "PACK %u/%u", static_cast<unsigned>(s.partnerCount),
                          static_cast<unsigned>(kMaxPartners));
            if (s.partnerCount == 0) {
                TextFit::addLine(lines, "Nobody yet.", Layout::kCardCols);
                TextFit::addLine(lines, "Beat a spirit", Layout::kCardCols);
                TextFit::addLine(lines, "and ask.", Layout::kCardCols);
            }
            for (uint8_t i = 0; i < s.partnerCount && i < kMaxPartners; ++i) {
                const SpeciesDef& sp = speciesDef(s.partners[i]);
                colors[lines.count] = Ui::affinityInk(static_cast<uint8_t>(sp.affinity));
                TextFit::addLine(lines, sp.name, Layout::kCardCols);
            }
            break;
        default:
            std::snprintf(mHeadText, sizeof(mHeadText), "BAG %u/%u", static_cast<unsigned>(bagCount(s)),
                          static_cast<unsigned>(kBagCapacity));
            for (ItemId i = 0; i < Item::Count; ++i) {
                if (s.bag[i] == 0) {
                    continue;
                }
                if (itemDef(i).key) {
                    TextFit::addLine(lines, itemDef(i).name, Layout::kCardCols);
                } else {
                    std::snprintf(line, sizeof(line), "%s x%u", itemDef(i).name, static_cast<unsigned>(s.bag[i]));
                    TextFit::addLine(lines, line, Layout::kCardCols);
                }
            }
            if (lines.count == 0) {
                TextFit::addLine(lines, "Empty.", Layout::kCardCols);
            }
            break;
    }
    lv_label_set_text_static(mHead, mHeadText);
    mCard.show(lines, colors[0], Ui::Ink::kText);
    for (uint8_t i = 1; i < lines.count; ++i) {
        mCard.setColor(i, colors[i]);
    }
}

void PackScreen::onKey(uint8_t code)
{
    if (code == Btn::R1) {
        mPage = static_cast<uint8_t>((mPage + 1u) % kPages);
        showPage();
    } else if (code == Btn::L2) {
        ScreenManager::instance().goTo(ScreenManager::screenFor(mModel.world()));
    }
}
