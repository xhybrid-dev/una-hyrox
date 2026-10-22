/**
 ******************************************************************************
 * @file    ProbeScreen.hpp
 * @brief   The Gate 0 heat probe's one screen: the temperature sensor's raw
 *          reading (big), its range and how often it reports; the heart rate
 *          with its trust; and the run's time, markers and log state.
 *
 *          R1 moves to the next phase (ambient at the start, then sauna,
 *          plunge, rest, sauna ...) and the log records it. The title is the
 *          phase; under the big number the heat hint says how the reading
 *          compares with the first minute (a rough guide, not a measurement).
 *          L1 starts the lowest and highest readings again.
 *          R2 exits.
 *
 *          The temperature is shown as the sensor reports it, with no unit:
 *          working out what the number means is what this screen is for.
 ******************************************************************************
 */

#ifndef PROBE_SCREEN_HPP
#define PROBE_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "ProbeSnapshot.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class ProbeScreen : public ModelListener
{
public:
    explicit ProbeScreen(Model& model);
    ~ProbeScreen();

    ProbeScreen(const ProbeScreen&)            = delete;
    ProbeScreen& operator=(const ProbeScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

    // ModelListener
    void onProbeUpdate(const Sauna::ProbeSnapshot& snapshot) override;

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    void show(const Sauna::ProbeSnapshot& s);

    Model&    mModel;
    lv_obj_t* mRoot    = nullptr;
    lv_obj_t* mTitle   = nullptr;
    lv_obj_t* mTemp    = nullptr;   ///< The big number
    lv_obj_t* mCaption = nullptr;   ///< What the number is
    lv_obj_t* mRange   = nullptr;   ///< "min .. max .."
    lv_obj_t* mHr      = nullptr;
    lv_obj_t* mTrust   = nullptr;
    lv_obj_t* mRate    = nullptr;   ///< Sample count and gap
    lv_obj_t* mStatus  = nullptr;   ///< Time, markers, log

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // PROBE_SCREEN_HPP
