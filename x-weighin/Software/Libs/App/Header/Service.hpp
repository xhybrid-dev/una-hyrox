/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Weigh-in background logic. W0: gather what the watch knows about
 *          the athlete (the UNA profile, the daily resting HR, the phone
 *          settings) and send it to the screen.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "AppConfigFields.hpp"
#include "Commands.hpp"

#include "SDK/AppConfig/AppConfig.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include <memory>

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    void onStartGUI();
    void onStopGUI();
    void onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch &data);

    /// Read the phone settings into mState.
    void loadConfiguration();

    /// Ask the kernel for the UNA profile (height, weight) into mState.
    void readProfile();

    void sendFirstLook();

    SDK::Kernel             &mKernel;
    /// The watch's daily heart-rate metrics, for the resting HR.
    SDK::Sensor::Connection  mSensorHrMetrics;

    /**
     * @brief   Read once when the service starts.
     *
     * Created in run(), not in the constructor. SDK::AppConfig logs when a file
     * is unusable, and in the simulator the service is constructed before the
     * logger can write, so logging that early crashes the simulator.
     */
    std::unique_ptr<SDK::AppConfig> mConfig;

    uint16_t mBirthYear;
    uint8_t  mBirthMonth;

    CustomMessage::FirstLookState mState;
    bool mGUIStarted;
};

#endif // SERVICE_HPP
