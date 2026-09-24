/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   What a screen can hear from the Model. Every hook defaults to nothing.
 ******************************************************************************
 */

#ifndef STREAK_MODEL_LISTENER_HPP
#define STREAK_MODEL_LISTENER_HPP

class ModelListener
{
public:
    virtual ~ModelListener() = default;

    /// The home view changed (a new message from the service, or the demo).
    virtual void onHomeView() {}

    /// No button for the screen timeout (Model::kScreenTimeoutSteps).
    virtual void onIdleTimeout() {}

    /// The GUI left the screen (the user opened another app or the launcher).
    virtual void onSuspend() {}
};

#endif // STREAK_MODEL_LISTENER_HPP
