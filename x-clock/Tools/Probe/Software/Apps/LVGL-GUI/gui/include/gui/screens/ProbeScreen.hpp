/**
 ******************************************************************************
 * @file    ProbeScreen.hpp
 * @brief   The Clock Probe's one screen (x-clock/docs/PROBE.md): a face.
 *
 * A face has one screen and no buttons (the kernel owns them), so the probe
 * shows everything at once:
 *   - on the rim, a ring of seven segments, Monday at twelve o'clock, each graded
 *     none / low / high by that day's activity (a DRAFT, from the family's training
 *     load; the grading is a placeholder, ActivityLevel.hpp), with a white mark
 *     over today;
 *   - the time;
 *   - seven rows, each with a light: the shared folder (files found, listing), the
 *     probe's own log, wrist raises, the GUI's ticks, today's plan from X Training,
 *     and X Run's VO2max with its trend.
 * Everything it draws comes from the service's view; it reads no file itself.
 ******************************************************************************
 */

#ifndef PROBE_SCREEN_HPP
#define PROBE_SCREEN_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class ProbeScreen : public ModelListener
{
public:
    explicit ProbeScreen(Model& model);
    ~ProbeScreen();

    ProbeScreen(const ProbeScreen&)            = delete;
    ProbeScreen& operator=(const ProbeScreen&) = delete;

    lv_obj_t* root() const { return mRoot; }

    void onChanged() override;

private:
    void draw();

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;
    lv_obj_t* mTime = nullptr;
    lv_obj_t* mFoot = nullptr;
    lv_obj_t* mDay[ClockProbe::kRingDays] = {};
    lv_obj_t* mToday = nullptr;
    lv_obj_t* mDot[ClockProbe::kRows] = {};
    lv_obj_t* mRow[ClockProbe::kRows] = {};
    lv_obj_t* mBar[ClockProbe::kTrendMax] = {};
};

#endif // PROBE_SCREEN_HPP
