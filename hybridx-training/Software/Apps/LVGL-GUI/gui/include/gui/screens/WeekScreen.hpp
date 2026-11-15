/**
 ******************************************************************************
 * @file    WeekScreen.hpp
 * @brief   X Training: This week (brief 8.1). Seven rows from Monday, each the
 *          day and its session's short name, coloured by how it stands (done,
 *          part done, today, to come, not logged, rest); the chosen row's date
 *          and state above them. L1/L2 choose a row, R1 opens its day, R2 goes
 *          back.
 ******************************************************************************
 */

#ifndef WEEK_SCREEN_HPP
#define WEEK_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class WeekScreen : public Screen
{
public:
    explicit WeekScreen(Model& model);
    ~WeekScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanWeek() override;

    /// The rows' centres and columns, for the text-fit audit.
    static constexpr int32_t kRowY0    = 78;   ///< Monday's centre
    static constexpr int32_t kRowPitch = 20;
    static constexpr int32_t kDayRight = 92;   ///< the weekday's right edge
    static constexpr int32_t kNameLeft = 102;  ///< the name's left edge

protected:
    void build() override;

private:
    void fill();
    void select(uint8_t row);

    lv_obj_t* mBand = nullptr;   ///< behind the chosen row
    lv_obj_t* mDay[7]  = {};
    lv_obj_t* mName[7] = {};
    lv_obj_t* mInfo  = nullptr;
    lv_obj_t* mEmpty[2] = {};
    uint8_t   mSel   = 0;
    bool      mOk    = false;
    char      mTitleText[20] = "THIS WEEK";
    char      mInfoText[40]  = "";
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // WEEK_SCREEN_HPP
