/**
 ******************************************************************************
 * @file    TrackSummaryScreen.hpp
 * @brief   Race summary (brief 8.2 item 7): an overview page, then the full
 *          split list five rows at a time.
 *
 * L1/L2 page, R2 leaves. No idle exit while a race is unsaved behind it.
 ******************************************************************************
 */

#ifndef TRACK_SUMMARY_SCREEN_HPP
#define TRACK_SUMMARY_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackSummaryScreen : public Screen
{
public:
    explicit TrackSummaryScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onSummary(const ActivitySummary& summary) override;

protected:
    void build() override;

private:
    /// Brief 8.2 item 7: five split rows a page.
    static constexpr uint8_t kRowsPerPage = 5;

    /// The overview needs a sixth row when Roxzone splitting is on, so the
    /// labels are allocated for six and the split pages hide the last one.
    static constexpr uint8_t kRowCount = 6;

    /// Page 0 is the overview; pages 1.. are the split list.
    uint8_t pageCount() const;
    void    redraw();

    uint8_t mPage = 0;

    /// Sets row @p i to a left name and a right value, or hides it when @p name
    /// is nullptr.
    void setRow(uint8_t i, const char* name, const char* value);

    lv_obj_t* mHeading = nullptr;
    lv_obj_t* mRowName[kRowCount] = {};
    lv_obj_t* mRowValue[kRowCount] = {};

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // TRACK_SUMMARY_SCREEN_HPP
