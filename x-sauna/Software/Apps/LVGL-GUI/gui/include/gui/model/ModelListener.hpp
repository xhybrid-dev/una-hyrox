/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "SessionSnapshot.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// No key for a while (a menu screen exits the app; the live screen never does).
    virtual void onIdleTimeout() {}

    /// The GUI is being suspended: drop anything half done, such as a hold.
    virtual void onSuspend() {}

    /// The service sent the session as it now stands (about once a second).
    virtual void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) { (void)snapshot; }

    /// A lap just ended: what it was, for the toast.
    virtual void onLapEvent(const Sauna::LapEventData& event) { (void)event; }

    /// The session ended (saved or not); what it came to.
    virtual void onSessionSaved(const Sauna::SavedSession& saved) { (void)saved; }

    /// The laps asked for by the summary.
    virtual void onLapsPage(const Sauna::LapsPageData& page) { (void)page; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
