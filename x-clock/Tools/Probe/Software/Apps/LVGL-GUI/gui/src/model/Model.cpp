/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   The Clock Probe's GUI model (see the header).
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);
}

void Model::frameTimerCb(lv_timer_t* timer)
{
    static_cast<Model*>(lv_timer_get_user_data(timer))->tick();
}

void Model::tick()
{
    ++mFrames;
    if (mResumed) {
        mResumed = false;
        ++mResumes;
        sendLife(1);
        // Whatever the kernel drew over the face is still in the framebuffer, and the service's last
        // messages may have been dropped while this GUI was off screen: ask for everything again.
        SDK::send_msg<CustomMessage::GuiRefresh>(mKernel);
    }
    if (mSuspended) {
        mSuspended = false;
        ++mSuspends;
        sendLife(2);
    }
    if (mFrames % kReportEvery == 0) {
        sendReport();
    }
}

void Model::sendLife(uint8_t kind)
{
    auto m = SDK::make_msg<CustomMessage::GuiLife>(mKernel);
    if (m) {
        m->kind   = kind;
        m->frames = mFrames;
        m.send();
    }
}

void Model::sendReport()
{
    auto m = SDK::make_msg<CustomMessage::GuiReport>(mKernel);
    if (m) {
        m->frames   = mFrames;
        m->resumes  = mResumes;
        m->suspends = mSuspends;
        m.send();
    }
}

bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::PROBE_VIEW:
            mView     = static_cast<CustomMessage::ProbeView*>(msg)->d;
            mHaveView = true;
            break;
        case CustomMessage::PROBE_CLOCK:
            mClock = static_cast<CustomMessage::ProbeClock*>(msg)->d;
            break;
        default:
            return true;
    }
    if (modelListener) {
        modelListener->onChanged();
    }
    return true;
}
