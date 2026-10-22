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

    /// The service sent the session as it now stands.
    virtual void onSessionUpdate(const Sauna::SessionSnapshot& snapshot) { (void)snapshot; }

    /// The session ended; what it came to.
    virtual void onSessionSaved(const Sauna::SavedSession& saved) { (void)saved; }

    /// The GUI is being suspended: drop anything half done, such as a hold.
    virtual void onSuspend() {}

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
