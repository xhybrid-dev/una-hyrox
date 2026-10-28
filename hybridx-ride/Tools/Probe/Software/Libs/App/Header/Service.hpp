/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   The bike probe's service (hybridx-ride/docs/PROBE.md): ask the
 *          kernel for each accessory kind in turn, and record what it says.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <memory>

#include "Commands.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel& kernel);
    virtual ~Service() = default;

    void run();

private:
    void nextRequest();
    void release();
    void onStatus(uint32_t kind, uint8_t state, const char* name);
    void onSensor(uint16_t handle, SDK::Sensor::DataBatch& data);

    /// The screen's log, and the CSV (Logs/probe_<utc>.csv in the app's folder).
    void line(const char* text);
    void csv(const char* event, uint32_t kind, int state, const char* name);
    void openCsv();
    void sendState();
    uint32_t now() const;

    SDK::Kernel&           mKernel;
    SDK::Sensor::Connection mSensorHr;
    std::unique_ptr<SDK::Interface::IFile> mCsv;

    uint32_t mStartMs    = 0;
    uint32_t mRequestMs  = 0;   ///< when the request in force was sent
    uint8_t  mStep       = 0xFF;
    uint8_t  mNext       = 0;
    uint8_t  mHrSource   = 0;
    uint8_t  mExtBpm     = 0;
    uint32_t mLastHrLog  = 0;
};

#endif // SERVICE_HPP
