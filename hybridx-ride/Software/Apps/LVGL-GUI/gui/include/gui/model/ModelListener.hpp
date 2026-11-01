/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "RideSnapshot.hpp"

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
    virtual void onSessionUpdate(const Ride::SessionSnapshot& snapshot) { (void)snapshot; }

    /// A lap just ended: what it was, for the toast.
    virtual void onLapEvent(const Ride::LapEventData& event) { (void)event; }

    /// The session ended (saved or not); what it came to.
    virtual void onSessionSaved(const Ride::SavedSession& saved) { (void)saved; }

    /// The laps asked for by the summary.
    virtual void onLapsPage(const Ride::LapsPageData& page) { (void)page; }

    /// The settings changed (the auto lap).
    virtual void onSettings() {}

    /// The strap's state changed.
    virtual void onStrap() {}

    /// The GPS state changed (searching, fix, off).
    virtual void onGps() {}

    /// The Streak line arrived, after a save.
    virtual void onStreakLine() {}

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
