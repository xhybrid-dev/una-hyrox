/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   The Clock Probe's GUI model: the last view and time the service
 *          sent, and the GUI's own account of its ticking.
 *
 * The probe's question about the GUI is "when does the kernel tick it?". So the
 * model counts frames (one LVGL timer firing per kernel tick), notes each resume
 * and suspend, and reports to the service: at once for a resume or suspend, and
 * every 50 frames otherwise. A report that does not arrive is the answer: the GUI
 * was not being ticked (Docs/service-lifecycle.md section 6).
 *
 * onResume() and onSuspend() may not be on the drawing thread (Docs/writing-a-clockface.md,
 * step 6), so they only raise a flag; the frame timer acts on it.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"
#include "ProbeView.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    /// A report goes to the service every this many frames (5 s at the watch's 10 a second).
    static constexpr uint32_t kReportEvery = 50;

    Model();

    void bind(ModelListener* listener) { modelListener = listener; }

    const ClockProbe::ViewData&  view() const { return mView; }
    const ClockProbe::ClockData& clock() const { return mClock; }
    bool                         haveView() const { return mHaveView; }

    /// LVGL calls this once per frame (a 1 ms timer fires on every lv_timer_handler call).
    static void frameTimerCb(lv_timer_t* timer);

protected:
    ModelListener*     modelListener;
    const SDK::Kernel& mKernel;

    ClockProbe::ViewData  mView {};
    ClockProbe::ClockData mClock {};
    bool                  mHaveView = false;

    volatile bool mResumed   = false;
    volatile bool mSuspended = false;
    uint32_t      mFrames    = 0;
    uint16_t      mResumes   = 0;
    uint16_t      mSuspends  = 0;

    void tick();
    void sendLife(uint8_t kind);
    void sendReport();

    void onStart() override {}
    void onResume() override { mResumed = true; }
    void onSuspend() override { mSuspended = true; }
    void onStop() override {}
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
