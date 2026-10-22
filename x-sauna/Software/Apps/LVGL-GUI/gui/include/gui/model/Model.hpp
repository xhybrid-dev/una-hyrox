/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of X Sauna and its link to the service.
 *
 * The service owns the session and sends it once a second and on every change
 * (Commands.hpp). The model keeps the latest copy so a screen can draw on first
 * show, and forwards each new one to the listener. Button actions are requests
 * to the service; the screen only ever shows what it last said.
 ******************************************************************************
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"

class ModelListener;

class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    /// The screen that receives model events. Exactly one is bound at a time.
    void bind(ModelListener* listener) { modelListener = listener; }

    /// Exit the application: the kernel stops both processes.
    void exitApp();

    void startSession(uint8_t mode, uint8_t rest);
    void lap();
    void pause();
    void resume();
    void stop(bool save);

    const Sauna::SessionSnapshot& snapshot() const { return mSnapshot; }

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    Sauna::SessionSnapshot mSnapshot {};

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
