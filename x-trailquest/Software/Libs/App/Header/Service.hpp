/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Trail Quest background logic. In Q0: judges timing-beat presses
 *          and plays feedback patterns; later it holds the game core and the
 *          save file.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "Commands.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"

#include <cstdint>

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    /// Ask the kernel to keep the backlight on for this long. A game is looked
    /// at between button presses, so the default timeout would turn it off.
    void holdBacklight(uint32_t timeoutMs);

    /// Vibro and buzzer for one XTrailQuest::FxId.
    void playFx(XTrailQuest::FxId id);

    void onBeatPress(const CustomMessage::BeatPress &press);

    SDK::Kernel &mKernel;
    XTrailQuest::BeatConfig mBeat;
};

#endif // SERVICE_HPP
