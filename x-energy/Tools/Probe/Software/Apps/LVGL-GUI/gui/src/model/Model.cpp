/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   The Energy Probe's GUI model (see the header).
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
    LOG_INFO("Keys: 1 = L1 (previous page), 2 = L2 (next page), 3 = R1 (the page's action), 4 = R2 (close)\n");
#endif
}

void Model::exitApp()
{
    // Only the screen closes: the service keeps logging (it never exits on GUI_STOP).
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

void Model::setOverride(uint8_t value)
{
    auto m = SDK::make_msg<CustomMessage::SetOverride>(mKernel);
    if (m) {
        m->value = value;
        m.send();
    }
}

void Model::setWindow(uint8_t value)
{
    auto m = SDK::make_msg<CustomMessage::SetWindow>(mKernel);
    if (m) {
        m->value = value;
        m.send();
    }
}

void Model::setDayMode(uint8_t value)
{
    auto m = SDK::make_msg<CustomMessage::SetDayMode>(mKernel);
    if (m) {
        m->value = value;
        m.send();
    }
}

void Model::startCheck()
{
    mCheckDone = false;
    mCheckCount = 0;
    SDK::send_msg<CustomMessage::StartCheck>(mKernel);
}

bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::PROBE_STATUS:
            mStatus = static_cast<CustomMessage::ProbeStatus*>(msg)->d;
            break;

        case CustomMessage::PROBE_NIGHTS: {
            mNights = static_cast<CustomMessage::ProbeNights*>(msg)->d;
            if (mNights.count > EnergyProbe::kHistory) {
                mNights.count = EnergyProbe::kHistory;
            }
        } break;

        case CustomMessage::PROBE_CHECK: {
            const auto* m = static_cast<CustomMessage::ProbeCheck*>(msg);
            if (m->count == 0) {
                mCheckSecs = m->secsLeft;
                mCheckDone = false;
                break;
            }
            for (uint8_t k = 0; k < m->count && k < CustomMessage::ProbeCheck::kPerMessage; ++k) {
                const uint8_t i = static_cast<uint8_t>(m->first + k);
                if (i >= kCheckLines) {
                    break;
                }
                std::memcpy(mCheck[i], m->text[k], sizeof(mCheck[i]));
                mCheck[i][sizeof(mCheck[i]) - 1] = '\0';
                if (i + 1u > mCheckCount) {
                    mCheckCount = static_cast<uint8_t>(i + 1u);
                }
            }
            mCheckSecs = 0;
            mCheckDone = true;
        } break;

        default:
            return true;
    }
    if (modelListener) {
        modelListener->onChanged();
    }
    return true;
}
