/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "Commands.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// A new home state arrived (at start, and after a save).
    virtual void onHome() {}

    /// New 30-day data arrived.
    virtual void onGraph() {}

    /// A save finished (the model holds the outcome).
    virtual void onSaveResult() {}

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
