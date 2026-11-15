/**
 ******************************************************************************
 * @file    PacesScreen.hpp
 * @brief   X Training: Paces (brief 8.1). Page 1 the paces; page 2 the times
 *          they come from; page 3 the race times they predict; page 4, when
 *          there is one, a note (threshold meets 10K pace; the half behind the
 *          5K; a half too old to use). L1/L2 turn the pages, R2 goes back.
 ******************************************************************************
 */

#ifndef PACES_SCREEN_HPP
#define PACES_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/TextStack.hpp"
#include "gui/widgets/Widgets.hpp"

class PacesScreen : public Screen
{
public:
    explicit PacesScreen(Model& model);
    ~PacesScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanPaces() override;

    /// Threshold within this of 10K pace, s/km: the note says they are about the same.
    static constexpr uint16_t kNearSPerKm = 5;

protected:
    void build() override;

private:
    void show(uint8_t page);
    uint8_t pages() const;
    /// The note's words, or "" for none.
    void note(char* buf, size_t cap) const;

    TextStack mStack;
    uint8_t   mPage = 0;
    char      mText[TextStack::kMaxLines][48] = {};
    char      mTitleText[24] = "PACES";
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // PACES_SCREEN_HPP
