/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   Knitting background logic: keep the counter, answer the GUI.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "Commands.hpp"
#include "RowCounter.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"

class Service
{
public:
    explicit Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    void sendUpdate();

    /// Write the counter to its file. Called after the screen has been told,
    /// so a slow write never delays the number on screen.
    void saveState();

    /// A short buzz, so a reset or an undo can be felt without looking.
    void buzz(uint8_t effect);

    SDK::Kernel          &mKernel;
    Knitting::RowCounter  mCounter;
};

#endif // SERVICE_HPP
