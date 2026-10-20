/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "ProbeSnapshot.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// The service sent the probe's readings.
    virtual void onProbeUpdate(const Sauna::ProbeSnapshot& snapshot) { (void)snapshot; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
