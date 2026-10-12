/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Weigh-in background logic: the weigh-in log, the trend, the
 *          athlete's fixed numbers, and saving a weigh-in.
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

#include <cstdint>
#include <ctime>
#include <memory>

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    /// The local date and time, and whether the clock can be trusted.
    struct Now {
        std::time_t utc     = 0;
        bool        clockOk = false;
        int32_t     day     = 0;   ///< Local day number.
        uint16_t    minute  = 0;   ///< Local minute of the day.
    };
    Now now() const;

    void onStartGUI();

    /// Read the phone settings.
    void loadConfiguration();
    /// Ask the kernel for the UNA profile (height, weight).
    void readProfile();
    /// Bring the log and its SharedData copy in step, then read the log
    /// into the trend.
    void loadLog();
    /// Log one weigh-in now and answer the GUI.
    void saveWeighIn(int32_t hundredths);

    void sendHome();
    void sendGraph();

    SDK::Kernel &mKernel;

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
    uint8_t  mSex;
    uint32_t mHeightCm;          ///< The UNA profile's height; 0 = not set.
    int32_t  mProfileHundredths; ///< The UNA profile's weight; 0 = not set.
    uint32_t mRejected;          ///< Log lines that did not parse.
    bool     mGUIStarted;
};

#endif // SERVICE_HPP
