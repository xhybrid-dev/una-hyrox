/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   The bike probe's GUI model: the log the service sends, kept here so
 *          the screen can scroll it.
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
    static constexpr uint8_t kLines = 32;   ///< kept for scrolling; older ones drop off

    Model();

    void bind(ModelListener* listener) { modelListener = listener; }
    void exitApp();
    void nextRequest();

    uint8_t     lineCount() const { return mCount; }
    /// i = 0 is the newest.
    const char* line(uint8_t i) const;
    const CustomMessage::ProbeState& state() const { return mState; }

protected:
    ModelListener*     modelListener;
    const SDK::Kernel& mKernel;

    char    mLines[kLines][40] {};
    uint8_t mNewest = 0;
    uint8_t mCount  = 0;
    CustomMessage::ProbeState mState {};

    void onStart() override {}
    void onResume() override {}
    void onSuspend() override {}
    void onStop() override {}
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
