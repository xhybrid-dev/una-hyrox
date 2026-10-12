/**
 ******************************************************************************
 * @file    Model.hpp
 * @brief   GUI-side state of X Weigh-in and its link to the service.
 *
 * The service owns the log, the trend and the settings. It sends the home
 * state and the last 30 days when the GUI starts and after every save, and
 * the outcome of each save (Commands.hpp). The model keeps the latest of each
 * so a screen can draw on first show, and forwards each new one to the screen
 * on display.
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

    /// Ask the service to log this weigh-in; SAVE_RESULT answers.
    void saveWeighIn(int32_t hundredths);

    bool                            hasHome() const { return mHasHome; }
    const CustomMessage::HomeData&  home() const { return mHome; }
    const XWeighIn::Window&         window() const { return mWindow; }
    CustomMessage::SaveOutcome      lastOutcome() const { return mLastOutcome; }
    int32_t                         lastSaved() const { return mLastSaved; }

    /// The page the pager shows next (Home, Graph, Recent, Body), kept here so
    /// a trip to the weigh-in screen comes back to Home but paging remembers.
    uint8_t page() const { return mPage; }
    void    setPage(uint8_t p) { mPage = p; }

protected:
    ModelListener*     modelListener;   ///< The screen on display
    const SDK::Kernel& mKernel;         ///< The GUI process's kernel interface

    bool                       mHasHome = false;
    CustomMessage::HomeData    mHome {};
    XWeighIn::Window           mWindow {};
    CustomMessage::SaveOutcome mLastOutcome = CustomMessage::SaveOutcome::WriteFailed;
    int32_t                    mLastSaved = 0;
    uint8_t                    mPage = 0;

    // IGuiLifeCycleCallback
    void onStart() override;
    void onResume() override;
    void onSuspend() override;
    void onStop() override;

    // ICustomMessageHandler: messages from the service
    bool customMessageHandler(SDK::MessageBase* msg) override;
};

#endif // MODEL_HPP
