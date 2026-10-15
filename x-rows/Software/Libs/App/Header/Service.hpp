/**
 ******************************************************************************
 * @file    Service.hpp
 * @brief   X Rows background logic: keep the counter, answer the GUI.
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

    SDK::Kernel     &mKernel;
    XRows::RowCounter mCounter;
};

#endif // SERVICE_HPP
