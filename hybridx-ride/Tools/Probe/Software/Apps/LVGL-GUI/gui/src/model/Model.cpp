/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   The bike probe's GUI model (see the header).
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include <cstring>

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
#if defined(SIMULATOR)
    LOG_INFO("Keys: 1 = L1 (newer), 2 = L2 (older), 3 = R1 (next request), 4 = R2 (exit)\n");
#endif
}

void Model::exitApp()
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::nextRequest()
{
    SDK::send_msg<CustomMessage::NextRequest>(mKernel);
}

const char* Model::line(uint8_t i) const
{
    if (i >= mCount) {
        return "";
    }
    return mLines[(mNewest + kLines - i) % kLines];
}

bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::PROBE_LINE: {
            const auto* m = static_cast<CustomMessage::ProbeLine*>(msg);
            mNewest = mCount == 0 ? 0 : static_cast<uint8_t>((mNewest + 1) % kLines);
            std::memcpy(mLines[mNewest], m->text, sizeof(mLines[mNewest]));
            mLines[mNewest][sizeof(mLines[mNewest]) - 1] = '\0';
            if (mCount < kLines) {
                ++mCount;
            }
            if (modelListener) {
                modelListener->onLog();
            }
        } break;

        case CustomMessage::PROBE_STATE: {
            const auto* m = static_cast<CustomMessage::ProbeState*>(msg);
            mState.step     = m->step;
            mState.next     = m->next;
            mState.hrSource = m->hrSource;
            mState.extBpm   = m->extBpm;
            mState.logging  = m->logging;
            if (modelListener) {
                modelListener->onState();
            }
        } break;

        default:
            break;
    }
    return true;
}
