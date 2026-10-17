/**
 ******************************************************************************
 * @file    Vo2PickerScreen.hpp
 * @brief   HybridX Run: pickers for the athlete's numbers (R5).
 *
 * One centred number in the faces and rows of RunLVGL's TwoTonePicker
 * (whose two columns cannot hold a four-digit year): the value, then the next
 * two above it in the list. Birth date is two stages, month then year; R1
 * moves on, R1 again saves. Max HR and resting HR are one stage, where "---"
 * is automatic (from age, or from the watch). L1 steps down, L2 up (RunLVGL's
 * convention); R1 saves; R2 goes back a stage or out without saving. Steps
 * and ranges: RunVo2::Settings (host-tested).
 ******************************************************************************
 */

#ifndef VO2_PICKER_SCREEN_HPP
#define VO2_PICKER_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class Vo2PickerScreen : public Screen
{
public:
    enum class Kind : uint8_t { Birth, MaxHr, RestHr };

    Vo2PickerScreen(Model& model, Kind kind);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    void render();
    void step(int dir);
    void save();
    uint8_t inUse() const;   ///< the value now in use, where stepping up from automatic starts

    Kind     mKind;
    bool     mAtYear = false;   ///< birth date: the second stage
    uint16_t mYear   = 0;
    uint8_t  mMonth  = 0;
    uint8_t  mValue  = 0;       ///< max or resting HR; 0 = automatic

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    lv_obj_t* mSub   = nullptr;
    lv_obj_t* mVal   = nullptr;
    lv_obj_t* mNext1 = nullptr;
    lv_obj_t* mNext2 = nullptr;
};

#endif // VO2_PICKER_SCREEN_HPP
