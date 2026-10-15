/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "RowCounter.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// The counter changed in the service.
    virtual void onCounterUpdate(const XRows::CounterState& state) { (void)state; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
