/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of Knitting and its link to the service.
 *
 * The service owns the counter and sends it on every change (Commands.hpp).
 * The model keeps the latest copy so a screen can draw on first show, and
 * forwards each new one to the listener. Button actions are requests to the
 * service; the screen only ever shows what the service last said.
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

    void countRow();
    void nextSet();
    void undoRow();

    /// A hold was cut short by the kernel after @p heldMs: write it to hold_diag.txt.
    void noteHoldCut(const char* button, uint32_t heldMs);
    void resetAll();

    /// The last counter from the service, so a screen can draw on first show.
    const Knitting::CounterState& counter() const { return mCounter; }

protected:
    /// Switch the kernel's music control off from this side as well (see Service).
    void setCapabilities();

    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    Knitting::CounterState mCounter {};

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
