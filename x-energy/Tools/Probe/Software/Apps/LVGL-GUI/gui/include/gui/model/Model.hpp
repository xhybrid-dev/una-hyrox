/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   The Energy Probe's GUI model: the last status, nights and check
 *          lines the service sent, and the requests the screen makes.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    static constexpr uint8_t kCheckLines = 24;

    Model();

    void bind(ModelListener* listener) { modelListener = listener; }
    void exitApp();

    // Requests to the service.
    void setOverride(uint8_t value);
    void setWindow(uint8_t value);
    void setDayMode(uint8_t value);
    void startCheck();

    const CustomMessage::StatusData& status() const { return mStatus; }
    const CustomMessage::NightsData& nights() const { return mNights; }
    uint8_t     checkCount() const { return mCheckCount; }
    uint8_t     checkSecsLeft() const { return mCheckSecs; }
    bool        checkDone() const { return mCheckDone; }
    const char* checkLine(uint8_t i) const { return i < mCheckCount ? mCheck[i] : ""; }

protected:
    ModelListener*     modelListener;
    const SDK::Kernel& mKernel;

    CustomMessage::StatusData mStatus {};
    CustomMessage::NightsData mNights {};
    char    mCheck[kCheckLines][40] {};
    uint8_t mCheckCount = 0;
    uint8_t mCheckSecs  = 0;
    bool    mCheckDone  = false;

    void onStart() override {}
    void onResume() override {}
    void onSuspend() override {}
    void onStop() override {}
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
